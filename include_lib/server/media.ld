
   _audio_decoder_begin = .;
   PROVIDE(audio_decoder_begin = .);
       *(.audio_decoder)
   _audio_decoder_end = .;
   PROVIDE(audio_decoder_end = .);

   _audio_package_begin = .;
   PROVIDE(audio_package_begin = .);
       *(.audio_package)
   _audio_package_end = .;
   PROVIDE(audio_package_end = .);

   _audio_hwaccel_begin = .;
   PROVIDE(audio_hwaccel_begin = .);
       *(.audio_hwaccel)
   _audio_hwaccel_end = .;
   PROVIDE(audio_hwaccel_end = .);

   _audio_output_handler_begin = .;
   PROVIDE(audio_output_handler_begin = .);
       *(.audio_output_handler)
   _audio_output_handler_end = .;
   PROVIDE(audio_output_handler_end = .);

   _audio_input_handler_begin = .;
   PROVIDE(audio_input_handler_begin = .);
       *(.audio_input_handler)
   _audio_input_handler_end = .;
   PROVIDE(audio_input_handler_end = .);

   _audio_subdev_begin = .;
   PROVIDE(audio_subdev_begin = .);
       *(.audio_subdev.0)
       *(.audio_subdev.1)
       *(.audio_subdev.2)
       *(.audio_subdev.3)
   _audio_subdev_end = .;
   PROVIDE(audio_subdev_end = .);

   _fm_dev_begin = .;
   PROVIDE(fm_dev_begin = .);
       *(.fm_dev)
   _fm_dev_end = .;
   PROVIDE(fm_dev_end = .);

    _video_dev_begin = .;
    PROVIDE(video_dev_begin = .);
        *(.video_device)
    _video_dev_end = .;
    PROVIDE(video_dev_end = .);

    _video_subdev_begin = .;
    PROVIDE(video_subdev_begin = .);
        *(.video_subdev.0)
        *(.video_subdev.1)
        *(.video_subdev.2)
        *(.video_subdev.3)
        *(.video_subdev.4)
        *(.video_subdev.5)
    _video_subdev_end = .;
    PROVIDE(video_subdev_end = .);

