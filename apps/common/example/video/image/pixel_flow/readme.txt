接口使用：

/**
 * @brief Computes pixel flow from image1 to image2
 *
 * Searches the corresponding position in the new image (image2) from the old image (image1)
 * and calculates the average offset of all.
 *
 * @param image1 previous image buffer
 * @param image2 current image buffer (new)

 *
 * @return quality of flow calculation， 表示置信度 0-255
 * 
 */
uint8_t compute_flow(uint8_t *image1, uint8_t *image2, int32_t *pixel_flow_x, int32_t *pixel_flow_y) 


1. 输出估算出来的偏移值，int32_t *pixel_flow_x, int32_t *pixel_flow_y，已经经过定点话，小数点4位，也可以通过宏FIXED_PRECISE来重定义精度；

2. 分辨率最大可支持到512x512, 最小是64x64, 性能在dv12大概是100fps, 性能与分辨率无关；

3. 为了得到较好的结果，建议是将输入图像720P, 扣出中间区域720x720，然后缩放成256x256, 再扔去计算，保证纹理的丰富性；



算法流程:

	1. 图像大小为64x64 - 512x512;
	2. 将图像分割为8x8子块;对每个子块做如下动作：
	3. 评价像素内部梯度(即相邻像素差异)；即描述纹理是否丰富；
	　　若纹理不够丰富，对该块不做统计；若纹理丰富，继续下面步骤；
	4. 寻找8x8子块的水平/垂直正负四个像素的最佳匹配块；
	5. 若最佳匹配块的相似度仍然太大，丢弃这个块；若最佳匹配块的匹配度足够高，继续下面步骤；
	6. 记录该块的匹配位移，并计算８个半像素位置的最佳匹配位置；其中半像素位置如下定义(p为整像素)
	      p  p  p  p  p
	　　　p　5  6  7  p
	      p  4  p  0  p
	      p  3  2  1  p
	      p  p  p  p  p
	      
	 7. 如果最佳匹配半像素位置0/1/7, 则匹配位移水平方向+1, 若为3/4/5,则-1;
	    如果最佳匹配半像素位置1/2/3, 则匹配位移垂直方向+1, 若为5/6/7,则-1;
	    将最终得到的半像素精度匹配位移做计数（直方图统计)；
	 
	 8.  重复3-7，直到所有子块做完，得到各种位移的计数；
	 
	 9.  用两个方法来得到最后的位移:
	     1) 所有位移技术的算术平均值；
	     2) 取计数最大的位移附近的几个计数做加权平均；
