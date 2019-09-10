// license:BSD-3-Clause
// copyright-holders:Brad Oliver,Aaron Giles,Bernd Wiebelt,Allard van der Bas
#ifndef __VECTOR__
#define __VECTOR__

#define VECTOR_COLOR111(c) \
	rgb_t(pal1bit((c) >> 2), pal1bit((c) >> 1), pal1bit((c) >> 0))

#define VECTOR_COLOR222(c) \
	rgb_t(pal2bit((c) >> 4), pal2bit((c) >> 2), pal2bit((c) >> 0))

#define VECTOR_COLOR444(c) \
	rgb_t(pal4bit((c) >> 8), pal4bit((c) >> 4), pal4bit((c) >> 0))

class vector_device;

/* The vertices are buffered here */
struct point
{
	point() :
		x(0),
		y(0),
		col(0),
		intensity(0) {}

	int x; int y;
	rgb_t col;
	int intensity;
};

class vector_options
{
public:
	friend class vector_device;

	static float s_flicker;
	static float s_beam_width_min;
	static float s_beam_width_max;
	static float s_beam_intensity_weight;

protected:
	static void init(emu_options& options);
};

class vector_device : public device_t, public device_video_interface
{
public:
	// construction/destruction
	vector_device(const machine_config &mconfig, const char *tag, device_t *owner, UINT32 clock);

	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void clear_list();

	void add_point(int x, int y, rgb_t color, int intensity);

	// device-level overrides
	virtual void device_start() override;

private:
	std::unique_ptr<point[]> m_vector_list;
	int m_vector_index;
	int m_min_intensity;
	int m_max_intensity;

	float normalized_sigmoid(float n, float k);
};

// device type definition
extern const device_type VECTOR;

#define MCFG_VECTOR_ADD(_tag) \
	MCFG_DEVICE_ADD(_tag, VECTOR, 0)

#endif
