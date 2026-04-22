// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <drm/drm_drv.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_modes.h>
#include <drm/drm_mipi_dbi.h>
#include <video/mipi_display.h>


static struct drm_driver st7735r_drm_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET,
	.name = "st7735r",
	.desc = "ST7735R LCD",
	.date = "2024",
	.major = 1,
	.minor = 0,
};

/* =========================
 * COMMANDS
 * ========================= */
#define ST7735R_MADCTL  0x36
#define ST7735R_COLMOD  0x3A

#define ST7735R_MX  0x40
#define ST7735R_MY  0x80
#define ST7735R_MV  0x20
#define ST7735R_BGR 0x08

struct st7735r_cfg {
	struct drm_display_mode mode;
	bool bgr;
	u16 x_offset;
	u16 y_offset;
};

struct st7735r_priv {
	struct mipi_dbi_dev dbidev;
	const struct st7735r_cfg *cfg;
};

/* =========================
 * ENABLE
 * ========================= */
static void st7735r_enable(struct drm_simple_display_pipe *pipe,
			   struct drm_crtc_state *crtc_state,
			   struct drm_plane_state *plane_state)
{
	struct mipi_dbi_dev *dbidev = drm_to_mipi_dbi_dev(pipe->crtc.dev);
	struct st7735r_priv *priv =
		container_of(dbidev, struct st7735r_priv, dbidev);
	struct mipi_dbi *dbi = &dbidev->dbi;
	u8 addr_mode = 0;
	int ret;

	ret = mipi_dbi_poweron_reset(dbidev);
	if (ret)
		return;

	msleep(150);

	mipi_dbi_command(dbi, MIPI_DCS_EXIT_SLEEP_MODE);
	msleep(120);

	/* rotation */
	addr_mode = ST7735R_MX | ST7735R_MY;
	if (priv->cfg->bgr)
		addr_mode |= ST7735R_BGR;

	mipi_dbi_command(dbi, ST7735R_MADCTL, addr_mode);
	mipi_dbi_command(dbi, ST7735R_COLMOD, 0x05); /* RGB565 */

	/* window */
	mipi_dbi_command(dbi, MIPI_DCS_SET_COLUMN_ADDRESS,
			 0x00, priv->cfg->x_offset,
			 0x00, priv->cfg->x_offset + 79);

	mipi_dbi_command(dbi, MIPI_DCS_SET_PAGE_ADDRESS,
			 0x00, priv->cfg->y_offset,
			 0x00, priv->cfg->y_offset + 159);

	mipi_dbi_command(dbi, MIPI_DCS_SET_DISPLAY_ON);
	msleep(50);

	mipi_dbi_enable_flush(dbidev, crtc_state, plane_state);
}

static const struct drm_simple_display_pipe_funcs st7735r_funcs = {
	DRM_MIPI_DBI_SIMPLE_DISPLAY_PIPE_FUNCS(st7735r_enable),
};

/* =========================
 * MODE FIX (QUAN TRỌNG)
 * ========================= */
static const struct drm_display_mode st7735r_mode = {
	.clock = 1,
	.hdisplay = 80,
	.hsync_start = 80,
	.hsync_end = 80,
	.htotal = 80,
	.vdisplay = 160,
	.vsync_start = 160,
	.vsync_end = 160,
	.vtotal = 160,
	.type = DRM_MODE_TYPE_DRIVER,
	.name = "80x160",
};

static const struct st7735r_cfg st7735r_096_cfg = {
	.mode = {
		.clock = 1,
		.hdisplay = 80,
		.hsync_start = 80,
		.hsync_end = 80,
		.htotal = 80,
		.vdisplay = 160,
		.vsync_start = 160,
		.vsync_end = 160,
		.vtotal = 160,
		.type = DRM_MODE_TYPE_DRIVER,
		.name = "80x160",
	},
	.bgr = true,
	.x_offset = 24,
	.y_offset = 0,
};

/* =========================
 * OF MATCH
 * ========================= */
static const struct of_device_id st7735r_of_match[] = {
	{ .compatible = "desmtiny,st7735r", .data = &st7735r_096_cfg },
	{ }
};
MODULE_DEVICE_TABLE(of, st7735r_of_match);

/* =========================
 * PROBE
 * ========================= */
static int st7735r_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct st7735r_priv *priv;
	struct mipi_dbi_dev *dbidev;
	int ret;

	priv = devm_drm_dev_alloc(dev, &st7735r_drm_driver,
				 struct st7735r_priv, dbidev.drm);
	if (IS_ERR(priv))
		return PTR_ERR(priv);

	dbidev = &priv->dbidev;
	priv->cfg = device_get_match_data(dev);

	ret = mipi_dbi_spi_init(spi, &dbidev->dbi, NULL);
	if (ret)
		return ret;

	ret = mipi_dbi_dev_init(dbidev, &st7735r_funcs,
				&priv->cfg->mode, 0);
	if (ret)
		return ret;

	return drm_dev_register(&dbidev->drm, 0);
}

/* =========================
 * SPI DRIVER
 * ========================= */
static struct spi_driver st7735r_spi_driver = {
	.driver = {
		.name = "st7735r",
		.of_match_table = st7735r_of_match,
	},
	.probe = st7735r_probe,
};
module_spi_driver(st7735r_spi_driver);

MODULE_DESCRIPTION("ST7735R 0.96 80x160 DRM driver");
MODULE_LICENSE("GPL");