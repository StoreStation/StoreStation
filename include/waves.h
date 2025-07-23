#pragma once

#define WAVES_THREADS 1
#define bufSize (1200)
#define linesAmt (3)
#define speed 2

typedef struct {
	bool completeFlags[WAVES_THREADS];
	int scrX = 480, scrY = 272;
	float ref = 0;
	float alphaMul = 1.0f;
	Vector3 buffer[WAVES_THREADS][bufSize][2];
	Color baseColor = {240, 144, 0, 0};
} Waves;

void Waves_drawLine(Waves * thizz, Vector3 v[2]) {
	unsigned char r = thizz->baseColor.r, g = thizz->baseColor.g, b = thizz->baseColor.b;
	const Vector2 vb = {v[0].x, v[0].y};
	const Vector2 ve = {v[1].x, v[1].y};
	const float cl = v[0].z*2;
	Color col = {(unsigned char) (r+(cl*2)), (unsigned char) (g+(cl*2)), 0*(unsigned char) (b+(cl*2)), (unsigned char) (40+(cl*4))};
	float vv = ((float) col.a + 100);
	if (ve.x > (float)thizz->scrX/3*2) {
		vv -= (ve.x - (float)thizz->scrX/3*2+1) / 25;
	}
	vv *= thizz->alphaMul;
	if (vv < 0) {
		vv = 0;
	}
	col.a = (unsigned char) vv;
	DrawLineEx(vb, ve, 15, col);
}

void Waves_populateBuffer(Waves * thizz, Vector3 vector2[bufSize][2], bool *done, const double ref) {
	const int sizeX = thizz->scrX + 100;
	const int yMod = thizz->scrY / 2;
	float lastPointX [linesAmt];
	float lastPointY [linesAmt];
	int c = 0;
	for (float cl = (float) 0; cl < linesAmt; cl++) { 
		const int skip = 20;
		for (int i = 0; i < sizeX; i += skip) {
			const auto x = (float) i;
			float ym = (sizeX - x + (ref - cl*30)) / skip / 6;
			ym *= 100;
			ym = (float) (int) ym / 100;
			ym = pspFpuFmod(ym, 2*3.14f);
			float y = (pspFpuSin(ym) * 150) / ((x+175)/thizz->scrX*1.75f) / 7.5;
			if (i != 0) {
				const int sizeY = 11;
				const float thisSize = sizeY - (cl * 4);
				for (float l = -thisSize; l < thisSize; l++) {
					vector2[c][0] = (Vector3) {
						.x = (float)(int)(lastPointX[(int)(cl)] - 10), .y = (float)(int)(lastPointY[(int)(cl)] + yMod + l*2), .z = (float)(cl)
					};
					vector2[c][1] = (Vector3) {
						.x = (float)(x - 10), .y = (float)(y + yMod + l*2), .z =  1
					};
					vector2[c+1][1].z = 0;
					c++;
				}
			}
			lastPointY[(int)(cl)] = (float) (int)(y);
			lastPointX[(int)(cl)] = (float) (int)(x);
		}
	}
	*done = true;
}

void Waves_lineWorker(Waves * thizz) {
	Waves_populateBuffer(thizz, thizz->buffer[0], &thizz->completeFlags[0], thizz->ref);
}

void Waves_drawBuffer(Waves * thizz, Vector3 b[bufSize][2]) {
	for (int i = 0; i < bufSize; i++) {
		if (b[i][1].z != 1) {
			return;
		}
		Waves_drawLine(thizz, b[i]);
	}
}

void Waves_draw(Waves * thizz) {
	Waves_drawBuffer(thizz, thizz->buffer[0]);
	thizz->ref+=speed;
}

void Waves_setAlphaPercent(Waves * thizz, float p) {
	if (p > 1) p = 1;
	else if (p < 0) p = 0;
	thizz->alphaMul = p;
}

float Waves_getAlphaPercent(Waves * thizz) {
	return thizz->alphaMul;
}