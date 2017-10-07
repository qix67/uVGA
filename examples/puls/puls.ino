#include <uVGA.h>

uVGA uvga;

//#define UVGA_DEFAULT_REZ
#define UVGA_240M_452X200
#include <uVGA_valid_settings.h>

UVGA_STATIC_FRAME_BUFFER(uvga_fb);

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
int HEIGHT = UVGA_FB_HEIGHT(UVGA_VREZ, UVGA_RPTL,UVGA_TOP_MARGIN, UVGA_BOTTOM_MARGIN);
boolean FISH_EYE_LENS = false;

int ITERATIONS = 256;
float GRID_SIZE = 256.0f;
float SPHERE_RADIUS = 64.0f;
float CYLINDER_RADIUS = 16.0f;
float Z0 = 320.0f;
float VELOCITY_X = 0.0f;
float VELOCITY_Y = -20.0f;
float VELOCITY_Z = 0.0f;
float VELOCITY_THETA = 0.002f;
float VELOCITY_PHI = 0.004f;

int HALF_WIDTH = WIDTH / 2;
int HALF_HEIGHT = HEIGHT / 2;
int PIXELS = WIDTH * HEIGHT;
float INVERSE_GRID_SIZE = 1.0f / GRID_SIZE;
float HALF_GRID_SIZE = GRID_SIZE / 2.0f;
float SPHERE_RADIUS_2 = SPHERE_RADIUS * SPHERE_RADIUS;
float INVERSE_SPHERE_RADIUS = 1.0f / SPHERE_RADIUS;
float CYLINDER_RADIUS_2 = CYLINDER_RADIUS * CYLINDER_RADIUS;
float INVERSE_CYLINDER_RADIUS = 1.0f / CYLINDER_RADIUS;

#define MATH_MAX(x,y)		((x) > (y) ? (x) : (y))

float ox = 0;
float oy = 0;
float oz = 0;

float phi = 0;
float theta = 0;

int fb_row_stride;

void loop()
{
	//int frame = 0;
	//long startTime = millis() - 1;

	fb_row_stride = UVGA_FB_ROW_STRIDE(UVGA_HREZ);

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

		int GX = (int)floorf(ox * INVERSE_GRID_SIZE);
		int GY = (int)floorf(oy * INVERSE_GRID_SIZE);
		int GZ = (int)floorf(oz * INVERSE_GRID_SIZE);

		float MAX = MATH_MAX(WIDTH, HEIGHT);
		float X_OFFSET = WIDTH < HEIGHT ? (HEIGHT - WIDTH) / 2 : 0;
		float Y_OFFSET = HEIGHT < WIDTH ? (WIDTH - HEIGHT) / 2 : 0;

		for(int s_h = 0; s_h < HEIGHT; s_h++)
		{
			uint8_t *fb = UVGA_LINE_ADDRESS(uvga_fb, fb_row_stride, s_h);

			for(int s_w = 0; s_w < WIDTH; s_w++)
			{
				float Rx, Ry, Rz;

				int red = 0;
				int green = 0;
				int blue = 0;

				int gx = GX;
				int gy = GY;
				int gz = GZ;

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
					float X = s_w - HALF_WIDTH + 0.5f;
					float Y = -(s_h - HALF_HEIGHT + 0.5f);

					Rx = X;
					Ry = Y;
					Rz = -Z0;

					float inverseMag = 1.0f / (float)sqrtf(Rx * Rx + Ry * Ry + Rz * Rz);

					Rx *= inverseMag;
					Ry *= inverseMag;
					Rz *= inverseMag;
				}

				float rx = ux * Rx + vx * Ry + wx * Rz;
				float ry = uy * Rx + vy * Ry + wy * Rz;
				float rz = uz * Rx + vz * Ry + wz * Rz;

				float irx = 1.0f / rx;
				float iry = 1.0f / ry;
				float irz = 1.0f / rz;

				int dgx = 0;
				int dgy = 0;
				int dgz = 0;

				float tx = 0;
				float ty = 0;
				float tz = 0;

				float dtx = fabsf(GRID_SIZE * irx);
				float dty = fabsf(GRID_SIZE * iry);
				float dtz = fabsf(GRID_SIZE * irz);

				if (rx > 0)
				{
					dgx = 1;
					tx = ((GRID_SIZE * (gx + 1)) - ox) * irx;
				}
				else
				{
					dgx = -1;
					tx = ((GRID_SIZE * gx) - ox) * irx;
				}
				if (ry > 0)
				{
					dgy = 1;
					ty = ((GRID_SIZE * (gy + 1)) - oy) * iry;
				}
				else
				{
					dgy = -1;
					ty = (GRID_SIZE * gy - oy) * iry;
				}
				if (rz > 0)
				{
					dgz = 1;
					tz = ((GRID_SIZE * (gz + 1)) - oz) * irz;
				}
				else
				{
					dgz = -1;
					tz = ((GRID_SIZE * gz) - oz) * irz;
				}

				for(int i = 0; i < ITERATIONS; i++)
				{

					float minT = MAXFLOAT;

					float minY = GRID_SIZE * gy;
					float maxY = minY + GRID_SIZE;

					float j = gx * GRID_SIZE + HALF_GRID_SIZE;
					float l = gz * GRID_SIZE + HALF_GRID_SIZE;
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
								float nx = ox + rx * t - j;
								float nz = oz + rz * t - l;
								float diffuse =	(nx * LIGHT_DIRECTION_X
								                   + nz * LIGHT_DIRECTION_Z);
								green = 128;
								if (diffuse > 0)
								{
									green += (int)(127.0f * diffuse * INVERSE_CYLINDER_RADIUS);
								}
								red = 0;
								blue = 0;
							}
						}
					}

					float minX = GRID_SIZE * gx;
					float maxX = minX + GRID_SIZE;

					j = gy * GRID_SIZE + HALF_GRID_SIZE;
					l = gz * GRID_SIZE + HALF_GRID_SIZE;
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
								float ny = oy + ry * t - j;
								float nz = oz + rz * t - l;
								float diffuse =	(ny * LIGHT_DIRECTION_Y
								                   + nz * LIGHT_DIRECTION_Z);
								red = 128;
								if (diffuse > 0)
								{
									red += (int)(127.0f * diffuse * INVERSE_CYLINDER_RADIUS);
								}
								green = red;
								blue = 0;
							}
						}
					}

					float minZ = GRID_SIZE * gz;
					float maxZ = minZ + GRID_SIZE;

					j = gy * GRID_SIZE + HALF_GRID_SIZE;
					l = gx * GRID_SIZE + HALF_GRID_SIZE;
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
								float ny = oy + ry * t - j;
								float nx = ox + rx * t - l;
								float diffuse =	(ny * LIGHT_DIRECTION_Y
								                   + nx * LIGHT_DIRECTION_X);
								blue = 128;
								if (diffuse > 0)
								{
									blue += (int)(127.0f * diffuse * INVERSE_CYLINDER_RADIUS);
								}
								red = 0;
								green = 0;
							}
						}
					}

					float sx = gx * GRID_SIZE + HALF_GRID_SIZE;
					float sy = gy * GRID_SIZE + HALF_GRID_SIZE;
					float sz = gz * GRID_SIZE + HALF_GRID_SIZE;

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
								red += (int)(127.0f * diffuse * INVERSE_SPHERE_RADIUS);
							}
							green = 0;
							blue = 0;
						}
					}

					if (tx < ty)
					{
						if (tx < tz)
						{
							tx += dtx;
							gx += dgx;
						}
						else
						{
							tz += dtz;
							gz += dgz;
						}
					}
					else if (ty < tz)
					{
						ty += dty;
						gy += dgy;
					}
					else
					{
						tz += dtz;
						gz += dgz;
					}
				}

				red = (red >> 5) & 0x7;
				green = (green >> 5) & 0x7;
				blue = (blue >> 6) & 0x3;

				*fb++ = (red << 5) | (green << 2) | blue;
			}
		}
	}
}
