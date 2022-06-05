#include <opencv2/opencv.hpp>

// 1. shift와 일반드래그 연결만 해주면 된다.
//		---> 변수누적을 풀고 전역행렬로 이어붙인다
//		---> drag 1, 2에서 모든 변수를 포함시킨다
// *** 모든 누적데이터는 변수에 들어있고 행렬은 껍데기일 뿐이다.


// 행렬 복사 함수
void copyMatrix(float IM[][3], float copy_IM[][3]) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			copy_IM[i][j] = IM[i][j];
}

// 기본행렬 초기화 함수
void setIdentityMatrix(float M[][3]) {
	M[0][0] = 1.0f;		M[0][1] = 0;		M[0][2] = 0;
	M[1][0] = 0;		M[1][1] = 1.0f;		M[1][2] = 0;
	M[2][0] = 0;		M[2][1] = 0;		M[2][2] = 1;
}

// 크기 변환 함수
void setScaleMatrix(float M[][3], float sx, float sy) {
	setIdentityMatrix(M); // 기본행렬로 초기화 해줌으로써 쓰레기값이 들어가지 않도록 방지
	M[0][0] = sx;		M[0][1] = 0; // x축으로 sx만큼 곱함
	M[1][0] = 0;		M[1][1] = sy; // y축으로 sy만큼 곱함
}


// 회전 함수
void setRotateMatrix(float M[][3], float theta) {
	setIdentityMatrix(M);
	float rad = theta * 3.141592 / 180.0f; // degree를 radian으로 변환

	M[0][0] = cos(rad);			M[0][1] = -sin(rad); // 회전행렬
	M[1][0] = sin(rad);			M[1][1] = cos(rad);
}

// 위치 이동 함수
void setTranslateMatrix(float M[][3], float tx, float ty) {
	setIdentityMatrix(M);
	M[0][2] = tx; // 이동 행렬 만들어줌
	M[1][2] = ty;
}

// 3X3 행렬의 곱을 만들어주는 함수
void setMultiplyMatrix(float M[][3], float A[][3], float B[][3]) {
	// M = A*B
	float temp1[3][3]; float temp2[3][3];
	copyMatrix(A, temp1);
	copyMatrix(B, temp2);
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			M[i][j] = 0; // 0으로 초기화
			for (int k = 0; k < 3; k++)
				M[i][j] += temp1[i][k] * temp2[k][j];
		}
}



// 변환함수 M을 src에 적용하여 변형된 사진인 dst을 만드는 함수
void applyAffineTransform(IplImage* src, IplImage* dst, float M[][3]) {
	cvSet(dst, cvScalar(0, 0, 0)); // 함수 사용 시 dst는 검은화면으로 초기화

	for (float y2 = 0; y2 < dst->height; y2++)
		for (float x2 = 0; x2 < dst->width; x2++) {
			float w2 = 1.0f;
			// 역행렬을 이용해서 변형하기 전의 점의 좌표(x1, y1)을 알아냄
			float x1 = M[0][0] * x2 + M[0][1] * y2 + M[0][2] * w2;
			float y1 = M[1][0] * x2 + M[1][1] * y2 + M[1][2] * w2;
			float w1 = M[2][0] * x2 + M[2][1] * y2 + M[2][2] * w2;
			if (x1 < 0 || x1 > src->width - 1) continue; // 이미지 바깥쪽 예외처리
			if (y1 < 0 || y1 > src->height - 1) continue;

			CvScalar d = cvGet2D(src, y1, x1);
			cvSet2D(dst, y2, x2, d);
		}
}



IplImage* src = cvLoadImage("c:\\temp\\lena.jpg");
IplImage* dst = cvCreateImage(cvGetSize(src), 8, 3);
IplImage* buf = cvCreateImage(cvGetSize(src), 8, 3);
float cx = dst->width / 2.0f;
float cy = dst->height / 2.0f;
CvPoint P = cvPoint(cx, cy);

float IM[3][3];
float copy_IM[3][3];

float theta = 0.0f;
float scale = 1.0f;
float tx = 0.f;
float ty = 0.f;


CvPoint pt1; // L버튼을 클릭한 지점을 기억하고 있는 전역변수
int draging = 0; // 그냥 드래그 하면 1, shift + 클릭 후 드래그 하면 2로 설정 

void myMouse(int event, int x, int y, int flags, void* param) {

	if (event == CV_EVENT_LBUTTONDOWN && (flags & CV_EVENT_FLAG_SHIFTKEY) == CV_EVENT_FLAG_SHIFTKEY) { // SHIFT를 누르면서 L버튼을 클릭할 시
		pt1 = cvPoint(x, y); // 클릭한 좌표를 저장
		draging = 2;

		return; // 아래에 그냥 L버튼을 클릭한 경우도 동작하는 것을 막기 위해 리턴으로 함수를 종료해 주어야 한다.
	}

	if (event == CV_EVENT_LBUTTONDOWN) {
		pt1 = cvPoint(x, y);
		draging = 1;
	}
	if (event == CV_EVENT_RBUTTONDOWN) {
		P = cvPoint(x, y);
		cvCopy(dst, buf);
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1);
		cvShowImage("dst", buf);
	}
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) == CV_EVENT_FLAG_LBUTTON) { // 드래그

		if (draging == 1) { // 일반 드래그 일 때
			printf("scale : %f theta : %f\n", scale, theta);
			CvPoint pt2 = cvPoint(x, y); // 마우스를 드래그할 때의 좌표를 pt2에 저장
			// 회전
			float theta1 = atan2(pt1.y - P.y, pt1.x - P.x); // atan을 이용하면 좌표에 따른 각도가 결과값으로 나옴
			float theta2 = atan2(pt2.y - P.y, pt2.x - P.x); // atan2인 이유는 tan범위 때문에 opencv에서 편의상 만든 함수임
			float M1[3][3];
			// 각도는 degree로 넣어야 하는데 atan2에서 나오는 값은 radian이라 변환이 필요
			theta += (theta2 - theta1) / 3.141592 * 180.0f; // 각도가 계속 누적이 되어야 현재 그림에서 각도 변경이 가능하다
			setRotateMatrix(M1, -theta); // 사진을 회전하는 IM행렬 생성

			// 확대축소
			float M2[3][3];
			float r1 = sqrt((pt1.x - P.x) * (pt1.x - P.x) + (pt1.y - P.y) * (pt1.y - P.y)); // pt1과 회전축 사이의 거리
			float r2 = sqrt((pt2.x - P.x) * (pt2.x - P.x) + (pt2.y - P.y) * (pt2.y - P.y)); // pt2와 회전축 사이의 거리
			scale *= (r2 / r1);
			setScaleMatrix(M2, 1.0f / scale, 1.0f / scale);

			// TM1은 그림의 중심을 원점으로 이동시키는 행렬
			// TM2는 다시 그림을 원위치로 이동시키는 행렬
			// M = TM2 M1 M2 TM1
			// IM = TM1 M2 M1 TM2
			float TM1[3][3];
			float TM2[3][3];
			float temp1[3][3];
			float temp2[3][3];
			float temp3[3][3];

			setTranslateMatrix(TM1, P.x, P.y);
			setTranslateMatrix(TM2, -P.x, -P.y);
			setMultiplyMatrix(temp1, TM1, M2);
			setMultiplyMatrix(temp2, M1, TM2);
			setMultiplyMatrix(IM, temp1, temp2);

			applyAffineTransform(src, dst, IM); // 변환행렬 적용

			cvCopy(dst, buf);
			cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1); // 회전축을 그려줌

			cvShowImage("dst", buf);

			pt1 = pt2; // 이걸 안해주면 드래그 시에 pt1이 처음상태에 고정되고 pt2는 누적적으로 커지므로 그림이 확확 커진다.
					   // pt1을 현재 위치인 pt2로 초기화를 해주어야 한다.

		}
		if (draging == 2) { // shift + L버튼 클릭 후 드래그 일 때
			// 여기서 중요한것은 사진의 위치가 변경될 때 회전축(P) 좌표를 함께 변동시켜 주어야 한다는 점이다.

			CvPoint pt2 = cvPoint(x, y); // 마우스를 드래그할 때의 좌표를 pt2에 저장
			tx += pt2.x - pt1.x; // 마우스를 클릭한 위치부터 떼는 위치까지의 x, y좌표 변화량만큼 사진이 이동해야 한다.
			ty += pt2.y - pt1.y;
			float TM[3][3];
			printf("tx : %f ty : %f\n", tx, ty);

			copyMatrix(IM, copy_IM);

			setTranslateMatrix(TM, -tx, -ty);

			applyAffineTransform(src, dst, TM);

			cvShowImage("dst", dst);
			pt1 = pt2;
		}
	}
}


int main() {

	cvCopy(src, dst);
	cvCopy(dst, buf);

	cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1);

	setIdentityMatrix(IM);


	cvShowImage("dst", buf);
	cvSetMouseCallback("dst", myMouse); // dst이미지에서 myMouse함수 적용 (암기) 
	cvWaitKey();

	return 0;
}