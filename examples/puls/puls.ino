#include <uVGA.h>

uVGA uvga;

#if F_CPU != 240000000
#pragma message "240MHz demo only"
#endif

#define UVGA_240M_452X240
#include <uVGA_valid_settings.h>

UVGA_STATIC_FRAME_BUFFER(uvga_fb);
UVGA_STATIC_FRAME_BUFFER(uvga_fb1);

DMAChannel cpydma;

DMABaseClass::TCD_t *cdma;
volatile uint8_t *cdmamux;
EDMA_REGs *edma = EDMA_ADDR;
int cdma_num;

void setup()
{
	int ret;
	DMABaseClass::TCD_t *edma_TCD;

	delay(1000);
	edma_TCD = (DMABaseClass::TCD_t*)&DMA_TCD0_SADDR;

	uvga.set_static_framebuffer(uvga_fb);
	ret = uvga.begin(&modeline);

	Serial.println(ret);

	if(ret != 0)
	{
		Serial.println("fatal error");
		while(1);
	}

	cpydma.begin(false);
	cdma_num = cpydma.channel;
	cdma = &(edma_TCD[cdma_num]);
	cdmamux = (volatile uint8_t *) & (DMAMUX0_CHCFG0) + cdma_num;

	*cdmamux = DMAMUX_ENABLE | DMAMUX_SOURCE_ALWAYS0;

	static volatile uint8_t *dma_chprio[DMA_NUM_CHANNELS] =
	{
		&DMA_DCHPRI0,  &DMA_DCHPRI1,  &DMA_DCHPRI2,  &DMA_DCHPRI3,
		&DMA_DCHPRI4,  &DMA_DCHPRI5,  &DMA_DCHPRI6,  &DMA_DCHPRI7,
		&DMA_DCHPRI8,  &DMA_DCHPRI9,  &DMA_DCHPRI10, &DMA_DCHPRI11,
		&DMA_DCHPRI12, &DMA_DCHPRI13, &DMA_DCHPRI14, &DMA_DCHPRI15
#if DMA_NUM_CHANNELS > 16
		, &DMA_DCHPRI16, &DMA_DCHPRI17, &DMA_DCHPRI18, &DMA_DCHPRI19,
		&DMA_DCHPRI20, &DMA_DCHPRI21, &DMA_DCHPRI22, &DMA_DCHPRI23,
		&DMA_DCHPRI24, &DMA_DCHPRI25, &DMA_DCHPRI26, &DMA_DCHPRI27,
		&DMA_DCHPRI28, &DMA_DCHPRI29, &DMA_DCHPRI30, &DMA_DCHPRI31
#endif
	};
	*dma_chprio[cdma_num] = *dma_chprio[cdma_num] | DMA_DCHPRI_ECP;

}


#define WIDTH (UVGA_HREZ)
#define HEIGHT (UVGA_FB_HEIGHT(UVGA_VREZ, UVGA_RPTL, UVGA_TOP_MARGIN, UVGA_BOTTOM_MARGIN))

//#define FISH_EYE_LENS

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

#define RED_BITS 8
#define GREEN_BITS 8
#define BLUE_BITS 8
#define RED_MIN 64
#define GREEN_MIN RED_MIN
#define BLUE_MIN RED_MIN

#define RED_RANGE  (((1 << RED_BITS) -1) - RED_MIN)
#define GREEN_RANGE  (((1 << GREEN_BITS) -1) - GREEN_MIN)
#define BLUE_RANGE  (((1 << BLUE_BITS) -1) - BLUE_MIN)

#define SPHERE_RADIUS_2 (SPHERE_RADIUS * SPHERE_RADIUS)
#define CYLINDER_RADIUS_2 (CYLINDER_RADIUS * CYLINDER_RADIUS)

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

	fb_row_stride = UVGA_FB_ROW_STRIDE(UVGA_HREZ);

	uvga.get_frame_buffer_size(&fb_width, &fb_height);

	memset(uvga_fb1, 0, sizeof(uvga_fb1));

#ifdef FISH_EYE_LENS
	float MAX = MATH_MAX(WIDTH, HEIGHT);
#endif

	while(true)
	{
		//long startTime = millis();

		float cosPhi = cosf(phi);
		float sinPhi = sinf(phi);
		float cosTheta = cosf(theta);
		float sinTheta = sinf(theta);

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

#ifdef FISH_EYE_LENS
		LIGHT_DIRECTION_X = -vx;
		LIGHT_DIRECTION_Y = -vy;
		LIGHT_DIRECTION_Z = -vz;

		float X_OFFSET = WIDTH < HEIGHT ? (HEIGHT - WIDTH) / 2 : 0;
		float Y_OFFSET = HEIGHT < WIDTH ? (WIDTH - HEIGHT) / 2 : 0;

#endif

		ox += VELOCITY_X;
		oy += VELOCITY_Y;
		oz += VELOCITY_Z;
		theta += VELOCITY_THETA;
		phi += VELOCITY_PHI;

		float GX = floorf(ox / GRID_SIZE) * GRID_SIZE;
		float GY = floorf(oy / GRID_SIZE) * GRID_SIZE;
		float GZ = floorf(oz / GRID_SIZE) * GRID_SIZE;

#ifndef FISH_EYE_LENS
		float Y = HEIGHT / 2  - 0.5f;
		float YY = Y * Y;
#endif

		float oxgs2 = ox - GRID_SIZE/2;
		float oygs2 = oy - GRID_SIZE/2;
		float ozgs2 = oz - GRID_SIZE/2;

		for(int s_h = 0; s_h < HEIGHT; s_h++)
		{
			uint8_t *fb = UVGA_LINE_ADDRESS(uvga_fb1, fb_row_stride, s_h);

#ifndef FISH_EYE_LENS
			float X = - WIDTH / 2 + 0.5f;
			float XX = X * X;
#endif

			for(int s_w = 0; s_w < WIDTH; s_w++)
			{
				float rx, ry, rz;

				float gx = GX;
				float gy = GY;
				float gz = GZ;

				{
					float Rx, Ry, Rz;

#ifdef FISH_EYE_LENS
					{
						float theta = (float)(PI * (0.5f + s_h + Y_OFFSET) / (float)MAX);
						float phi = (float)(PI * (0.5f + s_w + X_OFFSET) / (float)MAX);
						Rx = (float)(cosf(phi) * sinf(theta));
						Ry = (float)(sinf(phi) * sinf(theta));
						Rz = (float)(cosf(theta));
					}
#else
					{
						float Mag = sqrtf(XX + YY + Z0 * Z0);

						Rx = X / Mag;
						Ry = Y / Mag;
						Rz = -Z0 / Mag;
					}
#endif

					rx = ux * Rx + vx * Ry + wx * Rz;
					ry = uy * Rx + vy * Ry + wy * Rz;
					rz = uz * Rx + vz * Ry + wz * Rz;
				}

				float rxrx = rx * rx;
				float ryry = ry * ry;
				float rzrz = rz * rz;

				int object_hit;			// 1= light blue cylinder, 2 = yellow cylinder, 3 = light green cylinder, 4 = red sphere
				float diffuse_hit_t;
				float diffuse_hit_dx;
				float diffuse_hit_dy;
				float diffuse_hit_dz;

				for(int i = 0; i < ITERATIONS; i++)
				{
					float dx = oxgs2 - gx;
					float dy = oygs2 - gy;
					float dz = ozgs2 - gz;

					float dxdx = dx * dx;
					float dydy = dy * dy;
					float dzdz = dz * dz;

					float dxrx = dx * rx;
					float dyry = dy * ry;
					float dzrz = dz * rz;

					float gxmox = gx - ox;
					float gymoy = gy - oy;
					float gzmoz = gz - oz;

					object_hit = 0;
					diffuse_hit_t = MAXFLOAT;

					// hit light blue cylinder ?
					{
						float A = rxrx + rzrz;
						float B = (dxrx + dzrz);
						float C = dxdx + dzdz - CYLINDER_RADIUS_2;
						float D = B * B - A * C;
						if (D > 0)
						{
							float t = -(B + sqrtf(D)) / A;
							if (t > 0)
							{
								float y = ry * t;
								if (y >= gymoy && y <= (gymoy + GRID_SIZE))
								{
									diffuse_hit_t = t;
									diffuse_hit_dx = dx;
									diffuse_hit_dy = dy;
									diffuse_hit_dz = dz;

									object_hit = 1;
								}
							}
						}
					}

					// hit yellow cylinder ?
					{
						float A = ryry + rzrz;
						float B = (dyry + dzrz);
						float C = dydy + dzdz - CYLINDER_RADIUS_2;
						float D = B * B - A * C;
						if (D > 0)
						{
							float t = -(B + sqrtf(D)) / A;
							if (t > 0 && t < diffuse_hit_t)
							{
								float x = rx * t;
								if (x >= gxmox && x <= (gxmox + GRID_SIZE))
								{
									diffuse_hit_t = t;
									diffuse_hit_dx = dx;
									diffuse_hit_dy = dy;
									diffuse_hit_dz = dz;

									object_hit = 2;
								}
							}
						}
					}

					// hit light green cylinder ?
					{
						float A = ryry + rxrx;
						float B = (dyry + dxrx);
						float C = dydy + dxdx - CYLINDER_RADIUS_2;
						float D = B * B - A * C;
						if (D > 0)
						{
							float t = -(B + sqrtf(D)) / A;
							if (t > 0 && t < diffuse_hit_t)
							{
								float z = rz * t;
								if (z >= gzmoz && z <= (gzmoz + GRID_SIZE))
								{
									diffuse_hit_t = t;
									diffuse_hit_dx = dx;
									diffuse_hit_dy = dy;
									diffuse_hit_dz = dz;

									object_hit = 3;
								}
							}
						}
					}

					// hit red sphere ?
					{
						float B = dxrx + dyry + dzrz;
						float C = dxdx + dydy + dzdz - SPHERE_RADIUS_2;
						float D = B * B - C;
						if (D > 0)
						{
							float t = -B - sqrtf(D);
							if (t > 0 && t < diffuse_hit_t)
							{
								diffuse_hit_t = t;
								diffuse_hit_dx = dx;
								diffuse_hit_dy = dy;
								diffuse_hit_dz = dz;
								object_hit = 4;
								break;
							}
						}
					}

					if(diffuse_hit_t != MAXFLOAT)
						break;

					{
						float tx, ty, tz;

						tx = gx - ox;

						if(rx > 0)
							tx += GRID_SIZE;

						tx /= rx;

						ty = gy - oy;

						if (ry > 0)
							ty += GRID_SIZE;

						ty /= ry;

						tz = gz - oz;

						if (rz > 0)
							tz += GRID_SIZE;

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

				{
					uint8_t red = 0;
					uint8_t green = 0;
					uint8_t blue = 0;
					float diffuse_hit;
					float nx, ny ,nz;

					switch(object_hit)
					{
						case 1:	// hit light blue cylinder ?
									nx = diffuse_hit_dx + rx * diffuse_hit_t;
									nz = diffuse_hit_dz + rz * diffuse_hit_t;
									diffuse_hit =	(nx * LIGHT_DIRECTION_X + nz * LIGHT_DIRECTION_Z);

									red = 0;
									green = GREEN_MIN;
									if(diffuse_hit > 0)
										green += (int)(diffuse_hit * (GREEN_RANGE) / CYLINDER_RADIUS);

									blue = 255;
									break;

						case 2:	// hit yellow cylinder ?
									ny = diffuse_hit_dy + ry * diffuse_hit_t;
									nz = diffuse_hit_dz + rz * diffuse_hit_t;
									diffuse_hit =	(ny * LIGHT_DIRECTION_Y + nz * LIGHT_DIRECTION_Z);

									red = RED_MIN;
									if(diffuse_hit > 0)
										red += (int)(diffuse_hit * (RED_RANGE) / CYLINDER_RADIUS);

									green = red;
									blue = 0;
									break;

						case 3:	// hit light green cylinder ?
									ny = diffuse_hit_dy + ry * diffuse_hit_t;
									nx = diffuse_hit_dx + rx * diffuse_hit_t;
									diffuse_hit =	(ny * LIGHT_DIRECTION_Y + nx * LIGHT_DIRECTION_X);

									red = 0;
									green = GREEN_MIN;
									if (diffuse_hit > 0)
										green += (int)(diffuse_hit * (GREEN_RANGE) / CYLINDER_RADIUS);

									blue = green >> 1;
									break;

						case 4:	// hit red sphere ?
									nx = diffuse_hit_dx + rx * diffuse_hit_t;
									ny = diffuse_hit_dy + ry * diffuse_hit_t;
									nz = diffuse_hit_dz + rz * diffuse_hit_t;

									diffuse_hit =	(nx * LIGHT_DIRECTION_X + ny * LIGHT_DIRECTION_Y + nz * LIGHT_DIRECTION_Z);
									red = RED_MIN;
									if (diffuse_hit > 0)
										red += (int)(diffuse_hit * (RED_RANGE) / SPHERE_RADIUS);

									green = 0;
									blue = 0;
									break;
					}

					green = (green >> 5) & 0x7;
					blue = (blue >> 6) & 0x3;

					*fb++ = (red & (0x7 << 5)) | (green << 2) | blue;
				}
#ifndef FISH_EYE_LENS
				X++;
				XX = X * X;
#endif
			}
#ifndef FISH_EYE_LENS
			Y--;
			YY = Y * Y;
#endif
		}

		//Serial.println(millis() - startTime);

		// fast copy back buffer to front buffer using DMA
#if 0
		memcpy(UVGA_FB_START(uvga_fb, fb_row_stride), UVGA_FB_START(uvga_fb1, fb_row_stride), HEIGHT * fb_row_stride);
#else
		cdma->SADDR = UVGA_FB_START(uvga_fb1, fb_row_stride);      // source is first line of frame buffer partially in SRAM_U
		cdma->SOFF = 16;                                         // after each read, move source address 16 bytes forward
		cdma->ATTR_SRC = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);          // source data size = 16 bytes
		cdma->NBYTES = HEIGHT * fb_row_stride;                          // each minor loop transfers 1 framebuffer line
		cdma->SLAST = 0;

		cdma->DADDR = UVGA_FB_START(uvga_fb, fb_row_stride);                        // destination is the SRAM_L buffer
		cdma->DOFF = 16;                                         // after each write, move destination address 16 bytes forward
		cdma->ATTR_DST = DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_16BYTE);          // write data size = 16 bytes
		cdma->CITER = 1; // after each minor loop (except last one), start the 3rd DMA channel to fix DADDR of this TCD
		cdma->DLASTSGA = 0;                         // at end of major loop, let this TCD fixes itself instead of starting the 3rd DMA channel
		cdma->CSR = DMA_TCD_CSR_DREQ;
		cdma->BITER = cdma->CITER;
#endif
		uvga.waitBeam();
		edma->SERQ = cdma_num;

		// no need to wait end of DMA, first pixels will be copied before the CPU can compute their new values
	}
}
