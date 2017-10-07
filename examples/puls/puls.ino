#include <uVGA.h>

uVGA uvga;

//#define UVGA_DEFAULT_REZ
#define UVGA_240M_452X240
#include <uVGA_valid_settings.h>

UVGA_STATIC_FRAME_BUFFER(uvga_fb);
UVGA_STATIC_FRAME_BUFFER(uvga_fb1);

void setup()
{
	int ret;

	uvga.set_static_framebuffer(uvga_fb);
	ret = uvga.begin(&modeline);

	Serial.println(ret);

	if(ret != 0)
	{
		Serial.println("fatal error");
		while(1);
	}
}


int WIDTH = UVGA_HREZ;
int HEIGHT = UVGA_FB_HEIGHT(UVGA_VREZ, UVGA_RPTL, UVGA_TOP_MARGIN, UVGA_BOTTOM_MARGIN);
boolean FISH_EYE_LENS = false;

#define ITERATIONS 256
#define GRID_SIZE  256.0f
#define SPHERE_RADIUS 64.0f
#define CYLINDER_RADIUS 16.0f
#define Z0 320.0f
#define VELOCITY_X 0.0f
#define VELOCITY_Y -20.0f
#define VELOCITY_Z 0.0f
#define VELOCITY_THETA 0.002f
#define VELOCITY_PHI 0.004f

int PIXELS = WIDTH * HEIGHT;
float SPHERE_RADIUS_2 = SPHERE_RADIUS * SPHERE_RADIUS;
float CYLINDER_RADIUS_2 = CYLINDER_RADIUS * CYLINDER_RADIUS;

#define MATH_MAX(x,y)		((x) > (y) ? (x) : (y))

float ox = 0;
float oy = 0;
float oz = 0;

float phi = 0;
float theta = 0;

int fb_row_stride;
int fb_width;
int fb_height;

void loop()
{
	//int frame = 0;
	//long startTime = millis() - 1;

	fb_row_stride = UVGA_FB_ROW_STRIDE(UVGA_HREZ);

	uvga.get_frame_buffer_size(&fb_width, &fb_height);

	memset(uvga_fb1, 0, sizeof(uvga_fb1));

	float MAX = MATH_MAX(WIDTH, HEIGHT);

	while(true)
	{
		float cosPhi = (float)cosf(phi);
		float sinPhi = (float)sinf(phi);
		float cosTheta = (float)cosf(theta);
		float sinTheta = (float)sinf(theta);

		float ux = cosPhi * sinTheta;
		float uy = sinPhi * sinTheta;
		float uz = cosTheta;

		float vx = cosPhi * cosTheta;
		float vy = sinPhi * cosTheta;
		float vz = -sinTheta;

		float wx = uy * vz - uz * vy;
		float wy = uz * vx - ux * vz;
		float wz = ux * vy - uy * vx;

		float LIGHT_DIRECTION_X = wx;
		float LIGHT_DIRECTION_Y = wy;
		float LIGHT_DIRECTION_Z = wz;

		if (FISH_EYE_LENS)
		{
			LIGHT_DIRECTION_X = -vx;
			LIGHT_DIRECTION_Y = -vy;
			LIGHT_DIRECTION_Z = -vz;
		}

		ox += VELOCITY_X;
		oy += VELOCITY_Y;
		oz += VELOCITY_Z;
		theta += VELOCITY_THETA;
		phi += VELOCITY_PHI;
		/*
				if ((++frame & 0x3F) == 0)
				{
					setTitle((1000 * frame) / (millis() - startTime) + " fps");
				}
		*/

		int GX = (int)floorf(ox / GRID_SIZE);
		int GY = (int)floorf(oy / GRID_SIZE);
		int GZ = (int)floorf(oz / GRID_SIZE);

		float X_OFFSET = WIDTH < HEIGHT ? (HEIGHT - WIDTH) / 2 : 0;
		float Y_OFFSET = HEIGHT < WIDTH ? (WIDTH - HEIGHT) / 2 : 0;

		for(int s_h = 0; s_h < HEIGHT; s_h++)
		{
			uint8_t *fb = UVGA_LINE_ADDRESS(uvga_fb1, fb_row_stride, s_h);

			for(int s_w = 0; s_w < WIDTH; s_w++)
			{
				float rx, ry, rz;

				uint8_t red = 0;
				uint8_t green = 0;
				uint8_t blue = 0;

				int gx = GX * GRID_SIZE;
				int gy = GY * GRID_SIZE;
				int gz = GZ * GRID_SIZE;

				{
					float Rx, Ry, Rz;

					if (FISH_EYE_LENS)
					{
						float theta = (float)(PI * (0.5f + s_h + Y_OFFSET) / (float)MAX);
						float phi = (float)(PI * (0.5f + s_w + X_OFFSET) / (float)MAX);
						Rx = (float)(cosf(phi) * sinf(theta));
						Ry = (float)(sinf(phi) * sinf(theta));
						Rz = (float)(cosf(theta));
					}
					else
					{
						float X = s_w - WIDTH / 2 + 0.5f;
						float Y = -(s_h - HEIGHT / 2  + 0.5f);

						float Mag = (float)sqrtf(X * X + Y * Y + Z0 * Z0);

						Rx = X / Mag;
						Ry = Y / Mag;
						Rz = -Z0 / Mag;
					}

					rx = ux * Rx + vx * Ry + wx * Rz;
					ry = uy * Rx + vy * Ry + wy * Rz;
					rz = uz * Rx + vz * Ry + wz * Rz;
				}

				for(int i = 0; i < ITERATIONS; i++)
				{
					float j, l;

					float minT = MAXFLOAT;

					float minY = gy;
					float maxY = minY + GRID_SIZE;

					j = gx + (GRID_SIZE / 2);
					l = gz + (GRID_SIZE / 2);

					float P = ox - j;
					float Q = oz - l;
					float A = rx * rx + rz * rz;
					float B = 2 * (P * rx + Q * rz);
					float C = P * P + Q * Q - CYLINDER_RADIUS_2;
					float D = B * B - 4 * A * C;
					if (D > 0)
					{
						float t = (-B - (float)sqrtf(D)) / (2 * A);
						if (t > 0)
						{
							float y = oy + ry * t;
							if (y >= minY && y <= maxY)
							{
								minT = t;
								i = ITERATIONS;
								float nx = P + rx * t;
								float nz = Q + rz * t;
								float diffuse =	(nx * LIGHT_DIRECTION_X
								                   + nz * LIGHT_DIRECTION_Z);
								green = 128;
								if (diffuse > 0)
								{
									green += (int)(127.0f * diffuse / CYLINDER_RADIUS);
								}
								red = 0;
								blue = 0;
							}
						}
					}

					float minX = gx;
					float maxX = minX + GRID_SIZE;

					j = gy + (GRID_SIZE / 2);
					l = gz + (GRID_SIZE / 2);
					P = oy - j;
					Q = oz - l;
					A = ry * ry + rz * rz;
					B = 2 * (P * ry + Q * rz);
					C = P * P + Q * Q - CYLINDER_RADIUS_2;
					D = B * B - 4 * A * C;
					if (D > 0)
					{
						float t = (-B - (float)sqrtf(D)) / (2 * A);
						if (t > 0 && t < minT)
						{
							float x = ox + rx * t;
							if (x >= minX && x <= maxX)
							{
								minT = t;
								i = ITERATIONS;
								float ny = P + ry * t;
								float nz = Q + rz * t;
								float diffuse =	(ny * LIGHT_DIRECTION_Y
								                   + nz * LIGHT_DIRECTION_Z);
								red = 128;
								if (diffuse > 0)
								{
									red += (int)(127.0f * diffuse / CYLINDER_RADIUS);
								}
								green = red;
								blue = 0;
							}
						}
					}

					float minZ = gz;
					float maxZ = minZ + GRID_SIZE;

					j = gy + (GRID_SIZE / 2);
					l = gx + (GRID_SIZE / 2);
					P = oy - j;
					Q = ox - l;
					A = ry * ry + rx * rx;
					B = 2 * (P * ry + Q * rx);
					C = P * P + Q * Q - CYLINDER_RADIUS_2;
					D = B * B - 4 * A * C;
					if (D > 0)
					{
						float t = (-B - (float)sqrtf(D)) / (2 * A);
						if (t > 0 && t < minT)
						{
							float z = oz + rz * t;
							if (z >= minZ && z <= maxZ)
							{
								minT = t;
								i = ITERATIONS;
								float ny = P + ry * t;
								float nx = Q + rx * t;
								float diffuse =	(ny * LIGHT_DIRECTION_Y
								                   + nx * LIGHT_DIRECTION_X);
								blue = 128;
								if (diffuse > 0)
								{
									blue += (int)(127.0f * diffuse / CYLINDER_RADIUS);
								}
								red = 0;
								green = 0;
							}
						}
					}

					float sx = gx + (GRID_SIZE / 2);
					float sy = gy + (GRID_SIZE / 2);
					float sz = gz + (GRID_SIZE / 2);

					float dx = ox - sx;
					float dy = oy - sy;
					float dz = oz - sz;

					B = dx * rx + dy * ry + dz * rz;
					C = dx * dx + dy * dy + dz * dz - SPHERE_RADIUS_2;
					D = B * B - C;
					if (D > 0)
					{
						float t = -B - (float)sqrtf(D);
						if (t > 0 && t < minT)
						{
							i = ITERATIONS;
							float nx = ox + rx * t - sx;
							float ny = oy + ry * t - sy;
							float nz = oz + rz * t - sz;
							float diffuse =	(nx * LIGHT_DIRECTION_X
							                   + ny * LIGHT_DIRECTION_Y
							                   + nz * LIGHT_DIRECTION_Z);
							red = 128;
							if (diffuse > 0)
							{
								red += (int)(127.0f * diffuse / SPHERE_RADIUS);
							}
							green = 0;
							blue = 0;
						}
					}

					{
						float tx, ty, tz;

						tx = gx;

						if(rx > 0)
						{
							tx += GRID_SIZE;
						}

						tx -= ox;
						tx /= rx;

						ty = gy;

						if (ry > 0)
						{
							ty += GRID_SIZE;
						}

						ty -= oy;
						ty /= ry;


						tz = gz;

						if (rz > 0)
						{
							tz += GRID_SIZE;
						}

						tz -= oz;
						tz /= rz;


						if (tx < ty)
						{
							if (tx < tz)
							{
								if(rx > 0)
									gx += GRID_SIZE;
								else
									gx -= GRID_SIZE;
							}
							else
							{
								if(rz > 0)
									gz += GRID_SIZE;
								else
									gz -= GRID_SIZE;
							}
						}
						else if (ty < tz)
						{
							if(ry > 0)
								gy += GRID_SIZE;
							else
								gy -= GRID_SIZE;
						}
						else
						{
							if(rz > 0)
								gz += GRID_SIZE;
							else
								gz -= GRID_SIZE;
						}
					}
				}

				red = (red >> 5) & 0x7;
				green = (green >> 5) & 0x7;
				blue = (blue >> 6) & 0x3;

				*fb++ = (red << 5) | (green << 2) | blue;
			}
		}

		uvga.waitSync();
		memcpy(UVGA_FB_START(uvga_fb, fb_row_stride), UVGA_FB_START(uvga_fb1, fb_row_stride), sizeof(uvga_fb1));
	}
}