/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2020, OPEN AI LAB
 * Author: john2357@163.com
 *
 * original model: https://github.com/dog-qiuqiu/Yolo-Fastest/tree/master/ModelZoo/yolo-fastest-1.1_coco
 */


#ifdef _MSC_VER
#define NOMINMAX
#endif

#include "float.h"

#include "common.h"
#include "tengine/c_api.h"
#include "tengine_operations.h"


#include "app_config.h"

#define DEFAULT_REPEAT_COUNT 1
#define DEFAULT_THREAD_COUNT 1



static const char *class_names[] = {
    "background", "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light",
    "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow",
    "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
    "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
    "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone",
    "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase", "scissors", "teddy bear",
    "hair drier", "toothbrush"
};


static void get_input_data_darknet(const char *image_file, float *input_data, int net_h, int net_w)
{
    float mean[3] = {0.f, 0.f, 0.f};
    float scale[3] = {1.0f / 255, 1.0f / 255, 1.0f / 255};

    //no letter box by default
    get_input_data(image_file, input_data, net_h, net_w, mean, scale);
    // input rgb
    image swaprgb_img = {0};
    swaprgb_img.c = 3;
    swaprgb_img.w = net_w;
    swaprgb_img.h = net_h;
    swaprgb_img.data = input_data;
    rgb2bgr_permute(swaprgb_img);
}
void get_input_uint8_data(const char *image_file, uint8_t *input_data, int img_h, int img_w, float *mean, float *scale,
                          float input_scale, int zero_point)
{
    image img = imread_process(image_file, img_w, img_h, mean, scale);

    float *image_data = (float *)img.data;

    for (int i = 0; i < img_w * img_h * 3; i++) {
        int udata = (round)(image_data[i] / input_scale + zero_point);
        if (udata > 255) {
            udata = 255;
        } else if (udata < 0) {
            udata = 0;
        }

        input_data[i] = udata;
    }

    free_image(img);
}



int yolofastest(int argc, char *argv[])
{

    int repeat_count = DEFAULT_REPEAT_COUNT;
    int num_thread = DEFAULT_THREAD_COUNT;
    char *model_file = CONFIG_ROOT_PATH"yolo.txt";
    char *image_file = CONFIG_ROOT_PATH"dog.jpg";
    float mean[3] = {-1.f, -1.f, -1.f};
    float scale[3] = {0.f, 0.f, 0.f};

    int net_w = 320;
    int net_h = 320;


    /* check files */
    if (NULL == model_file) {
        printf("Error: Tengine model file not specified!\n");
        show_usage();
        return -1;
    }

    if (NULL == image_file) {
        printf("Error: Image file not specified!\n");
        show_usage();
        return -1;
    }
    printf("%s   %d\n", __func__, __LINE__);
    if (!check_file_exist(model_file) || !check_file_exist(image_file)) {
        printf("%s   %d\n", __func__, __LINE__);
        return -1;
    }
    /* set runtime options */
    struct options opt;
    opt.num_thread = num_thread;
    opt.cluster = TENGINE_CLUSTER_ALL;
    opt.precision = TENGINE_MODE_UINT8;
    opt.affinity = 0;

    /* inital tengine */
    if (init_tengine() != 0) {
        printf("Initial tengine failed.\n");
        return -1;
    }

    printf("tengine-lite library version: %s\n", get_tengine_version());

    /* create graph, load tengine model xxx.tmfile */
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb1111");
    graph_t graph = create_graph(NULL, "tengine", model_file);
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb222222");
    if (graph == NULL)



    {
        printf("Create graph failed.\n");
        return -1;
    }

    /* set the input shape to initial the graph, and prerun graph to infer shape */
    /* set the shape, data buffer of input_tensor of the graph */
    int img_size = net_h * net_w * 3;
    int dims[] = {1, 3, net_h, net_w}; // nchw
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb33333");
    uint8_t *input_data = (uint8_t *)malloc(img_size);

    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb444444");
    tensor_t input_tensor = get_graph_input_tensor(graph, 0, 0);
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb55555");
    if (input_tensor == NULL) {
        printf("Get input tensor failed\n");
        return -1;
    }

    if (set_tensor_shape(input_tensor, dims, 4) < 0) {
        printf("Set input tensor shape failed\n");
        return -1;
    }

    if (set_tensor_buffer(input_tensor, input_data, img_size) < 0) {
        printf("Set input tensor buffer failed\n");
        return -1;
    }

    /* prerun graph, set work options(num_thread, cluster, precision) */
    if (prerun_graph_multithread(graph, opt) < 0) {
        printf("Prerun multithread graph failed.\n");
        return -1;
    }

    /* prepare process input data, set the data mem to input tensor */
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb1111");
    float input_scale = 0.f;
    int input_zero_point = 0;
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb22222");
    get_tensor_quant_param(input_tensor, &input_scale, &input_zero_point, 1);
    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb3333");
    get_input_uint8_data(image_file, input_data, net_h, net_w, mean, scale, input_scale, input_zero_point);

    printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbb44444");
    //get_input_data_darknet(image_file, input_data, net_h, net_w);

    /* run graph */
    double min_time = DBL_MAX;
    double max_time = DBL_MIN;
    double total_time = 0.;
    for (int i = 0; i < repeat_count; i++) {
        double start = get_current_time();
        if (run_graph(graph, 1) < 0) {
            printf("Run graph failed\n");
            return -1;
        }
        double end = get_current_time();
        double cur = end - start;
        total_time += cur;
        if (min_time > cur) {
            min_time = cur;
        }
        if (max_time < cur) {
            max_time = cur;
        }
    }
    printf("Repeat %d times, thread %d, avg time %.2f ms, max_time %.2f ms, min_time %.2f ms\n", repeat_count,
           num_thread, total_time / repeat_count, max_time, min_time);
    printf("--------------------------------------\n");

    /* get the result of classification */

    int output_node_num = get_graph_output_node_number(graph);
    printf("output_node_num=%d\n", output_node_num);



//    tensor_t output_tensor = get_graph_output_tensor(graph, 0, 0);
//    float* output_data = (float*)get_tensor_buffer(output_tensor);
//    int output_size = get_tensor_buffer_size(output_tensor) / sizeof(float);
//
//    print_topk(output_data, output_size, 5);
    printf("--------------------------------------\n");
    /* process the detection result */
//    image img = imread(image_file);
//    std::vector<BBoxRect> boxes;
//    run_yolo(graph, boxes, img.w, img.h);
//
//    for (int i = 0; i < (int)boxes.size(); ++i)
//    {
//        BBoxRect b = boxes[i];
//        //draw_box(img, b.xmin, b.ymin, b.xmax, b.ymax, 2, 125, 0, 125);
//        printf("%2d: %3.0f%%, [%4.0f, %4.0f, %4.0f, %4.0f], %s\n", b.label, b.score * 100, b.xmin, b.ymin, b.xmax, b.ymax, class_names[b.label]);
//    }
//    save_image(img, "yolofastest_out");
//
//    /* release tengine */
//    free_image(img);

    release_graph_tensor(input_tensor);
    postrun_graph(graph);
    destroy_graph(graph);
    release_tengine();

    return 0;
}

int fprintf(FILE *stream, const char *format, ...)
{
    int ret;
    va_list param;
    va_start(param, format);
    ret = vfprintf(stream, format, param);
    va_end(param);
    return ret;
}



