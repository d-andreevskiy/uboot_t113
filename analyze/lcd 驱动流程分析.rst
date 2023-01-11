
1. 定义CONFIG_VIDEO 之后，sunxi-common.h 中有如下定义

#define CONSOLE_STDOUT_SETTINGS \
	"stdout=serial,vidconsole\0" \
	"stderr=serial,vidconsole\0"

console_init_r 中有如下调用：
if (OVERWRITE_CONSOLE == 0) {	/* if not overwritten by config switch */
    inputdev  = console_search_dev(DEV_FLAGS_INPUT,  stdinname);
    outputdev = console_search_dev(DEV_FLAGS_OUTPUT, stdoutname);
    errdev    = console_search_dev(DEV_FLAGS_OUTPUT, stderrname);
    if (CONFIG_IS_ENABLED(CONSOLE_MUX)) {
        iomux_err = iomux_doenv(stdin, stdinname);
        iomux_err += iomux_doenv(stdout, stdoutname);
        iomux_err += iomux_doenv(stderr, stderrname);
        if (!iomux_err)
            /* Successful, so skip all the code below. */
            goto done;
    }
}

会调用iomux_doenv：此函数的作用就是逐个解析stdout stderr 中的参数，并且调用函数 console_search_dev

struct stdio_dev *console_search_dev(int flags, const char *name)
{
	struct stdio_dev *dev;

	dev = stdio_get_by_name(name);
#ifdef CONFIG_VIDCONSOLE_AS_LCD
	if (!dev && !strcmp(name, CONFIG_VIDCONSOLE_AS_NAME))
		dev = stdio_get_by_name("vidconsole");
#endif

	if (dev && (dev->flags & flags))
		return dev;

	return NULL;
}

console_search_dev 调用 stdio_get_by_name

struct stdio_dev *stdio_get_by_name(const char *name)
{
	struct list_head *pos;
	struct stdio_dev *sdev;

	if (!name)
		return NULL;

	list_for_each(pos, &devs.list) {
		sdev = list_entry(pos, struct stdio_dev, list);
		if (strcmp(sdev->name, name) == 0)
			return sdev;
	}
	if (IS_ENABLED(CONFIG_VIDEO)) {
		/*
		 * We did not find a suitable stdio device. If there is a video
		 * driver with a name starting with 'vidconsole', we can try
		 * probing that in the hope that it will produce the required
		 * stdio device.
		 *
		 * This function is sometimes called with the entire value of
		 * 'stdout', which may include a list of devices separate by
		 * commas. Obviously this is not going to work, so we ignore
		 * that case. The call path in that case is
		 * console_init_r() -> console_search_dev() -> stdio_get_by_name()
		 */
		if (!strncmp(name, "vidconsole", 10) && !strchr(name, ',') &&
		    !stdio_probe_device(name, UCLASS_VIDEO, &sdev))
			return sdev;
	}

	return NULL;
}

如果出现 vidconsole ，则会调用 stdio_probe_device ， vidconsole 在 stdout 和 stderr 中定义

stdio_probe_device 中调用 uclass_get_device_by_seq 
uclass_get_device_by_seq 找到定义的 sunxi_de2 驱动，并调用 sunxi_de2_probe

函数的调用流程
console_init_r
    -> iomux_doenv
        -> console_search_dev(... , "vidconsole")
            -> stdio_get_by_name("vidconsole")
                -> stdio_probe_device(name, UCLASS_VIDEO, &sdev)
                    -> uclass_get_device_by_seq
                        -> sunxi_de2_probe

经过上面的流程，后面就涉及到T113的流程了


2. sunxi_de2_probe 函数调用流程分析

static int sunxi_de2_probe(struct udevice *dev)
{
    ......

	ret = uclass_get_device_by_driver(UCLASS_DISPLAY,
					  DM_DRIVER_GET(sunxi_lcd), &disp);

    ......
}
HDMI 部分的代码不分析， T113 不支持 HDMI
sunxi_de2_probe 如上调用，找到 sunxi_lcd 并且调用 sunxi_lcd_probe

static int sunxi_lcd_probe(struct udevice *dev)
{
    ....

	ret = uclass_get_device(UCLASS_PANEL, 0, &cdev);

	if (fdtdec_decode_display_timing(gd->fdt_blob, dev_of_offset(cdev),
					 0, &priv->timing)) {
		debug("%s: Failed to decode display timing\n", __func__);
		return -EINVAL;
	}
	timing_node = fdt_subnode_offset(gd->fdt_blob, dev_of_offset(cdev),
					 "display-timings");
	node = fdt_first_subnode(gd->fdt_blob, timing_node);
	val = fdtdec_get_int(gd->fdt_blob, node, "bits-per-pixel", -1);
	if (val != -1)
		priv->panel_bpp = val;
	else
		priv->panel_bpp = 18;

	return 0;
}

sunxi_lcd_probe 调用 uclass_get_device(UCLASS_PANEL, 0, &cdev)

在设备树中添加了如下定义:
panel-rgb@0 {
    compatible = "simple-panel";
    backlight = <&backlight>;

    display-timings {
        timing@0 {
            clock-frequency = <50000000>;
            hactive = <480>;
            vactive = <272>;
            hfront-porch = <2>;
            hback-porch = <2>;
            hsync-len = <41>;
            vfront-porch = <2>;
            vback-porch = <2>;
            vsync-len = <10>;
            hsync-active = <0>;
            vsync-active = <0>;
            de-active = <0>;
            pixelclk-active = <1>;
            bits-per-pixel = <16>;
        };
    };
};

找到 drivers/video/simple_panel.c 里面 compatible = "simple-panel"
sunxi_lcd_probe 会找到 simple_panel 并调用函数 simple_panel_of_to_plat 和 simple_panel_probe
没有在设备树中定义 reg，simple_panel_probe没有调用关系
simple_panel_of_to_plat
    -> uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev, "backlight", &priv->backlight);
会调用 backlight，就是背光驱动，这个留待后面分析。

sunxi_lcd_probe 调用 fdtdec_decode_display_timing，用来获取设备树下面 display-timings 的参数，并保存在 priv->timing 中
sunxi_lcd_probe 调用 fdtdec_get_int(gd->fdt_blob, node, "bits-per-pixel", -1) ，用来设置 priv->panel_bpp

sunxi_lcd_probe
    -> uclass_get_device(UCLASS_PANEL, 0, &cdev)
        -> simple_panel_probe
    -> fdtdec_decode_display_timing
    -> fdt_subnode_offset
    -> fdt_first_subnode
    -> fdtdec_get_int(gd->fdt_blob, node, "bits-per-pixel", -1)

sunxi_lcd_probe 两个作用：填充 priv->timing 和设置 priv->panel_bpp

sunxi_de2_probe 调用 sunxi_de2_init
static int sunxi_de2_init(struct udevice *dev, ulong fbbase,
			  enum video_log2_bpp l2bpp,
			  struct udevice *disp, int mux, bool is_composite)
{
    /* 获取 disp 的 plat */
	disp_uc_plat = dev_get_uclass_plat(disp);

	disp_uc_plat->source_id = mux;

    /* 读取 timing 的值 */
	ret = display_read_timing(disp, &timing);

    /* 时钟初始化 */
	sunxi_de2_composer_init();
    /* 设置 de */
	sunxi_de2_mode_set(mux, &timing, 1 << l2bpp, fbbase, is_composite);
    /* 设置 tcon */
	ret = display_enable(disp, 1 << l2bpp, &timing);

	uc_priv->xsize = timing.hactive.typ;
	uc_priv->ysize = timing.vactive.typ;
	uc_priv->bpix = l2bpp;
}

sunxi_de2_probe 调用 video_set_flush_dcache 刷新 lcd 的存储区域

sunxi_de2_probe
    -> sunxi_lcd_probe
    -> sunxi_de2_init
    -> video_set_flush_dcache


3. 接下来分析一下 backlight 背光功能，MQ_DUAL默认背光常亮
simple_panel_of_to_plat
    -> uclass_get_device_by_phandle(UCLASS_PANEL_BACKLIGHT, dev, "backlight", &priv->backlight)
        -> pwm_backlight_of_to_plat
            -> dev_read_phandle_with_args(dev, "pwms", "#pwm-cells", 0, 0, &args);      
            -> uclass_get_device_by_ofnode(UCLASS_PWM, args.node, &priv->pwm);          /* 这两行用来寻找 pwm 驱动 */
            -> dev_read_u32_default(dev, "default-brightness-level", 255);
            -> dev_read_prop(dev, "brightness-levels", &len);                           /* 读取 backlight 等级 */
        -> pwm_backlight_probe （空）

sunxi_lcd_enable 中如下调用 会使能 PWM
	ret = uclass_get_device(UCLASS_PANEL_BACKLIGHT, 0, &backlight);
	if (!ret)
		backlight_enable(backlight);

