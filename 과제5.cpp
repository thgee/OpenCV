#include <opencv2/opencv.hpp>
#include <iostream>

char path[100]; // 이미지의 경로를 입력받을 변수
IplImage* src;
IplImage* dst;
IplImage* buf;

// 행렬 복사 함수
void copyMatrix(float M[][3], float copy_M[][3]) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			copy_M[i][j] = M[i][j];
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
void setRotateMatrix(float M[][3], float theta) { // theta는 atan2에서 나온 결과값으로 radian 값이므로 바로 적용 가능
	setIdentityMatrix(M);


	M[0][0] = cos(theta);			M[0][1] = -sin(theta); // 회전행렬
	M[1][0] = sin(theta);			M[1][1] = cos(theta);
}

// 위치 이동 함수
void setTranslateMatrix(float M[][3], float tx, float ty) {
	setIdentityMatrix(M);
	M[0][2] = tx; // 이동 행렬 만들어줌
	M[1][2] = ty;
}

// 3X3 행렬의 곱을 만들어주는 함수
// 이 함수 사용시 주의할점 : setMultiplyMatrix(M1, M1, M2) 이런식으로 같은 행렬을 두번 넣어주면 안된다
// 왜냐하면 인자로 배열의 주소값이 들어가기 때문에 +=로 중첩되는 과정에서 증가한 배열이 다시 함수에 들어가기 때문에 정상적인 연산이 안된다
void setMultiplyMatrix(float M[][3], float A[][3], float B[][3]) {
	// M = A*B
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			M[i][j] = 0; // 행렬을 0으로 초기화
			for (int k = 0; k < 3; k++)
				M[i][j] += A[i][k] * B[k][j];
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


// 원점이동TM1 -> 확대축소SM -> 회전RM -> 사진이동TM -> 원위치이동TM2
float cx; // 이미지의 중심좌표
float cy;
CvPoint P; // 피봇좌표

float IM[3][3]; // 최종 변환행렬
float copy_IM[3][3]; // 행렬IM을 복사한 행렬

float theta = 0.0f; // 회전각도 (degree)
float scale = 1.0f; // 확대축소 비율
float tx = 0.f; // 이미지 이동 정도
float ty = 0.f;

float RM[3][3]; // 회전행렬
float SM[3][3]; // 확대축소행렬
float TM[3][3]; // 이동행렬
float TM1[3][3]; // 피봇 ->원점 이동행렬
float TM2[3][3]; // 원점 -> 피봇 이동행렬
float temp1[3][3]; // 행렬을 곱할 때 사용되는 temp행렬들
float temp2[3][3];
float temp3[3][3];
float temp4[3][3];

float rem[3][3]; // 이전 피봇에서의 변환을 기억하는 행렬

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
		P = cvPoint(x, y); // 오른쪽버튼 클릭 시 피봇좌표를 저장


		theta = 0.0f; // 변수값들 초기화
		scale = 1.0f; // 초기화를 해도 되는 이유는 이미 rem행렬에 이전까지의 변환행렬들의 곱을 저장했기 때문
		tx = 0.f;
		ty = 0.f;


		copyMatrix(IM, rem); // IM을 rem에 저장

		cvCopy(dst, buf); // buf이미지에 피봇지점을 찍고 이미지를 출력함으로써 dst이미지는 깨끗한 상태를 유지한다
						  // buf를 이용하지 않고 dst에 직접 피봇을 찍으면 피봇을 재설정 할 때마다 이전의 점이 지워지지 않고 계속 늘어난다
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1);
		cvShowImage("dst", buf);
	}
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) == CV_EVENT_FLAG_LBUTTON) { // 드래그

		if (draging == 1) { // 일반 드래그 일 때
			CvPoint pt2 = cvPoint(x, y); // 마우스를 드래그할 때의 좌표를 pt2에 저장
			// 회전
			float theta1 = atan2(pt1.y - P.y, pt1.x - P.x); // 피봇좌표와 현재 클릭한 좌표의 x차이, y차이를 atan2함수에 넣어서 그 각도를 얻어온다
			float theta2 = atan2(pt2.y - P.y, pt2.x - P.x); // atan2는 atan의 tan범위를 고려하여 opencv에서 편의상 만든 함수임
			theta += (theta2 - theta1);// 각도가 계속 누적이 되어야 src로부터 회전한 각도를 theta가 기억하여 연속적인 변환이 가능하다

			// 확대축소
			float r1 = sqrt((pt1.x - P.x) * (pt1.x - P.x) + (pt1.y - P.y) * (pt1.y - P.y)); // pt1과 피봇 사이의 거리
			float r2 = sqrt((pt2.x - P.x) * (pt2.x - P.x) + (pt2.y - P.y) * (pt2.y - P.y)); // pt2와 피봇 사이의 거리
			scale *= (r2 / r1);

			pt1 = pt2; // 이걸 안해주면 드래그 시에 pt1이 처음상태에 고정되고 pt2는 누적적으로 커지므로 그림이 확확 커진다.
					   // pt1을 현재 위치인 pt2로 초기화를 해줌으로써 딱 마우스를 이동하는 만큼씩만 누적되도록 한다
		}
		if (draging == 2) { // shift + L버튼 클릭 후 드래그 일 때

			CvPoint pt2 = cvPoint(x, y); // 마우스를 드래그할 때의 좌표를 pt2에 저장

			tx += (pt2.x - pt1.x); // 마우스를 클릭한 위치부터 떼는 위치까지의 x, y좌표 변화량만큼 사진이 이동해야 한다.
			ty += (pt2.y - pt1.y);
			P.x += (pt2.x - pt1.x); // 피봇도 함께 이동해야 한다
			P.y += (pt2.y - pt1.y);

			pt1 = pt2;
		}

		// TM1은 피봇좌표를 원점으로 이동시키는 행렬
		// TM2는 피봇좌표를 다시 원위치하는 행렬

		setRotateMatrix(RM, -theta); // 사진을 회전하는 IM행렬 생성
		setScaleMatrix(SM, 1.0f / scale, 1.0f / scale);
		setTranslateMatrix(TM1, P.x, P.y);
		setTranslateMatrix(TM2, -P.x, -P.y);
		setTranslateMatrix(TM, -tx, -ty);

		// 행렬 곱하는 순서
		//	 M = TM2 RM SM TM TM1 rem 정방향
		//	 IM = rem TM1 TM SM RM TM2 역방향

		setMultiplyMatrix(temp1, rem, TM1);
		setMultiplyMatrix(temp2, TM, SM);
		setMultiplyMatrix(temp3, RM, TM2);
		setMultiplyMatrix(temp4, temp1, temp2);
		setMultiplyMatrix(IM, temp4, temp3);

		applyAffineTransform(src, dst, IM); // 변환행렬 적용

		cvCopy(dst, buf);
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1); // 피봇을 그려줌

		cvShowImage("dst", buf);
	}
}


int main() {

	std::cout << "Left Mouse Dragging : Rotating / Scaling" << std::endl;
	std::cout << "Left Mouse Dragging + Shiftkey : Translating" << std::endl;
	std::cout << "Right Mouse Click : Resetting the pivot point" << std::endl;
	std::cout << "=============================================" << std::endl;
	std::cout << "InputFile Path : ";
	std::cin >> path;
	src = cvLoadImage(path);
	dst = cvCreateImage(cvGetSize(src), 8, 3);
	buf = cvCreateImage(cvGetSize(src), 8, 3);

	cx = dst->width / 2.0f;
	cy = dst->height / 2.0f;
	P = cvPoint(cx, cy); // 피봇의 초기값을 이미지의 중심으로 설정

	cvCopy(src, dst);

	// 변환행렬들을 기본행렬로 초기화
	// 초깃값을 기본행렬로 안해주면 shift나 Lbutton 을 맨 처음 클릭 시에 multiplyMatrix에 이상한 행렬이 들어가서 변환이 비정상적으로 된다.
	setIdentityMatrix(IM);
	setIdentityMatrix(rem);
	setIdentityMatrix(RM);
	setIdentityMatrix(SM);
	setIdentityMatrix(TM);
	setIdentityMatrix(TM1);
	setIdentityMatrix(TM2);

	cvShowImage("dst", dst); // 처음 화면엔 피봇이 그려지지 않은 이미지가 출력
	cvSetMouseCallback("dst", myMouse); // dst이미지에서 myMouse함수 적용
	cvWaitKey();

	return 0;
}