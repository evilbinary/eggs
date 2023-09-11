target("etk")
    set_kind("static")

    add_packages("musl")
    add_deps("gui")

    add_files(
     'etk_rect.c',
    'etk_stack.c',
    'etk_widget.c',
    'etk_window.c',
    'etk_wnd_manager.c',
    'etk_display.c',
    'etk_bitmap.c',
    'etk_color.c',
    'etk_platform.c',
    'etk_canvas.c',
    'etk_gconf.c',
    'etk_font.c',
    'etk_event.c',
    'etk_image_decoder.c',
    'etk.c',
    'etk_global.c',
    'etk_main_loop.c',
    'etk_util.c',
    'etk_image_jpeg_decoder.c',
    'etk_entry.c',
    'etk_text_buffer.c',
    'etk_input_method_manager.c',
    'etk_input_method.c',
    'ekt_input_method_pinyin.c',
    'etk_button.c',
    'etk_menu.c',
    'etk_menu_item.c',
    'etk_image_bmp_decoder.c',
    'etk_source_timer.c',
    'etk_source.c',
    'etk_sources_manager.c',
    'etk_check_button.c',
    'etk_group_box.c',
    'etk_canvas_ex.c'
    )
    add_cflags('-DLIBYC')
    
    add_includedirs(
        '.',
        {public= true}
    )