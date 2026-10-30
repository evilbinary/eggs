/*
 * This file registers the FreeType modules compiled into the library.
 *
 * If you use GNU make, this file IS NOT USED!  Instead, it is created in
 * the objects directory (normally `<topdir>/objs/`) based on information
 * from `<topdir>/modules.cfg`.
 *
 * Please read `docs/INSTALL.ANY` and `docs/CUSTOMIZE` how to compile
 * FreeType without GNU make.
 *
 */

FT_USE_MODULE( FT_Module_Class, autofit_module_class )
FT_USE_MODULE( FT_Driver_ClassRec, tt_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, t1_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, cff_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, t1cid_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, pfr_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, t42_driver_class )
FT_USE_MODULE( FT_Driver_ClassRec, winfnt_driver_class )
/* YiYiYa：只编了这些模块对应的 src 子目录，pcf/bdf/sdf/svg 没纳入
 * （sdf/svg 是 2.13+ 新增的渲染器，svg 还要额外依赖）。未编入的一律注释，
 * 否则 ft_init 表引用到不存在的 xxx_driver_class ⇒ 未定义符号。 */
//FT_USE_MODULE( FT_Driver_ClassRec, pcf_driver_class )
//FT_USE_MODULE( FT_Driver_ClassRec, bdf_driver_class )
FT_USE_MODULE( FT_Module_Class, psaux_module_class )
FT_USE_MODULE( FT_Module_Class, psnames_module_class )
FT_USE_MODULE( FT_Module_Class, pshinter_module_class )
FT_USE_MODULE( FT_Module_Class, sfnt_module_class )
FT_USE_MODULE( FT_Renderer_Class, ft_smooth_renderer_class )
FT_USE_MODULE( FT_Renderer_Class, ft_raster1_renderer_class )
//FT_USE_MODULE( FT_Renderer_Class, ft_sdf_renderer_class )
//FT_USE_MODULE( FT_Renderer_Class, ft_bitmap_sdf_renderer_class )
//FT_USE_MODULE( FT_Renderer_Class, ft_svg_renderer_class )

/* EOF */
