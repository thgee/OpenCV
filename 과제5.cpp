#include <opencv2/opencv.hpp>
#include <iostream>

char path[100]; // �̹����� ��θ� �Է¹��� ����
IplImage* src;
IplImage* dst;
IplImage* buf;

// ��� ���� �Լ�
void copyMatrix(float M[][3], float copy_M[][3]) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			copy_M[i][j] = M[i][j];
}

// �⺻��� �ʱ�ȭ �Լ�
void setIdentityMatrix(float M[][3]) {
	M[0][0] = 1.0f;		M[0][1] = 0;		M[0][2] = 0;
	M[1][0] = 0;		M[1][1] = 1.0f;		M[1][2] = 0;
	M[2][0] = 0;		M[2][1] = 0;		M[2][2] = 1;
}

// ũ�� ��ȯ �Լ�
void setScaleMatrix(float M[][3], float sx, float sy) {
	setIdentityMatrix(M); // �⺻��ķ� �ʱ�ȭ �������ν� �����Ⱚ�� ���� �ʵ��� ����
	M[0][0] = sx;		M[0][1] = 0; // x������ sx��ŭ ����
	M[1][0] = 0;		M[1][1] = sy; // y������ sy��ŭ ����
}

// ȸ�� �Լ�
void setRotateMatrix(float M[][3], float theta) { // theta�� atan2���� ���� ��������� radian ���̹Ƿ� �ٷ� ���� ����
	setIdentityMatrix(M);


	M[0][0] = cos(theta);			M[0][1] = -sin(theta); // ȸ�����
	M[1][0] = sin(theta);			M[1][1] = cos(theta);
}

// ��ġ �̵� �Լ�
void setTranslateMatrix(float M[][3], float tx, float ty) {
	setIdentityMatrix(M);
	M[0][2] = tx; // �̵� ��� �������
	M[1][2] = ty;
}

// 3X3 ����� ���� ������ִ� �Լ�
// �� �Լ� ���� �������� : setMultiplyMatrix(M1, M1, M2) �̷������� ���� ����� �ι� �־��ָ� �ȵȴ�
// �ֳ��ϸ� ���ڷ� �迭�� �ּҰ��� ���� ������ +=�� ��ø�Ǵ� �������� ������ �迭�� �ٽ� �Լ��� ���� ������ �������� ������ �ȵȴ�
void setMultiplyMatrix(float M[][3], float A[][3], float B[][3]) {
	// M = A*B
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			M[i][j] = 0; // ����� 0���� �ʱ�ȭ
			for (int k = 0; k < 3; k++)
				M[i][j] += A[i][k] * B[k][j];
		}
}

// ��ȯ�Լ� M�� src�� �����Ͽ� ������ ������ dst�� ����� �Լ�
void applyAffineTransform(IplImage* src, IplImage* dst, float M[][3]) {
	cvSet(dst, cvScalar(0, 0, 0)); // �Լ� ��� �� dst�� ����ȭ������ �ʱ�ȭ

	for (float y2 = 0; y2 < dst->height; y2++)
		for (float x2 = 0; x2 < dst->width; x2++) {
			float w2 = 1.0f;
			// ������� �̿��ؼ� �����ϱ� ���� ���� ��ǥ(x1, y1)�� �˾Ƴ�
			float x1 = M[0][0] * x2 + M[0][1] * y2 + M[0][2] * w2;
			float y1 = M[1][0] * x2 + M[1][1] * y2 + M[1][2] * w2;
			float w1 = M[2][0] * x2 + M[2][1] * y2 + M[2][2] * w2;
			if (x1 < 0 || x1 > src->width - 1) continue; // �̹��� �ٱ��� ����ó��
			if (y1 < 0 || y1 > src->height - 1) continue;

			CvScalar d = cvGet2D(src, y1, x1);
			cvSet2D(dst, y2, x2, d);
		}
}


// �����̵�TM1 -> Ȯ�����SM -> ȸ��RM -> �����̵�TM -> ����ġ�̵�TM2
float cx; // �̹����� �߽���ǥ
float cy;
CvPoint P; // �Ǻ���ǥ

float IM[3][3]; // ���� ��ȯ���
float copy_IM[3][3]; // ���IM�� ������ ���

float theta = 0.0f; // ȸ������ (degree)
float scale = 1.0f; // Ȯ����� ����
float tx = 0.f; // �̹��� �̵� ����
float ty = 0.f;

float RM[3][3]; // ȸ�����
float SM[3][3]; // Ȯ��������
float TM[3][3]; // �̵����
float TM1[3][3]; // �Ǻ� ->���� �̵����
float TM2[3][3]; // ���� -> �Ǻ� �̵����
float temp1[3][3]; // ����� ���� �� ���Ǵ� temp��ĵ�
float temp2[3][3];
float temp3[3][3];
float temp4[3][3];

float rem[3][3]; // ���� �Ǻ������� ��ȯ�� ����ϴ� ���

CvPoint pt1; // L��ư�� Ŭ���� ������ ����ϰ� �ִ� ��������
int draging = 0; // �׳� �巡�� �ϸ� 1, shift + Ŭ�� �� �巡�� �ϸ� 2�� ���� 

void myMouse(int event, int x, int y, int flags, void* param) {

	if (event == CV_EVENT_LBUTTONDOWN && (flags & CV_EVENT_FLAG_SHIFTKEY) == CV_EVENT_FLAG_SHIFTKEY) { // SHIFT�� �����鼭 L��ư�� Ŭ���� ��
		pt1 = cvPoint(x, y); // Ŭ���� ��ǥ�� ����
		draging = 2;

		return; // �Ʒ��� �׳� L��ư�� Ŭ���� ��쵵 �����ϴ� ���� ���� ���� �������� �Լ��� ������ �־�� �Ѵ�.
	}

	if (event == CV_EVENT_LBUTTONDOWN) {
		pt1 = cvPoint(x, y);
		draging = 1;
	}
	if (event == CV_EVENT_RBUTTONDOWN) {
		P = cvPoint(x, y); // �����ʹ�ư Ŭ�� �� �Ǻ���ǥ�� ����


		theta = 0.0f; // �������� �ʱ�ȭ
		scale = 1.0f; // �ʱ�ȭ�� �ص� �Ǵ� ������ �̹� rem��Ŀ� ���������� ��ȯ��ĵ��� ���� �����߱� ����
		tx = 0.f;
		ty = 0.f;


		copyMatrix(IM, rem); // IM�� rem�� ����

		cvCopy(dst, buf); // buf�̹����� �Ǻ������� ��� �̹����� ��������ν� dst�̹����� ������ ���¸� �����Ѵ�
						  // buf�� �̿����� �ʰ� dst�� ���� �Ǻ��� ������ �Ǻ��� �缳�� �� ������ ������ ���� �������� �ʰ� ��� �þ��
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1);
		cvShowImage("dst", buf);
	}
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) == CV_EVENT_FLAG_LBUTTON) { // �巡��

		if (draging == 1) { // �Ϲ� �巡�� �� ��
			CvPoint pt2 = cvPoint(x, y); // ���콺�� �巡���� ���� ��ǥ�� pt2�� ����
			// ȸ��
			float theta1 = atan2(pt1.y - P.y, pt1.x - P.x); // �Ǻ���ǥ�� ���� Ŭ���� ��ǥ�� x����, y���̸� atan2�Լ��� �־ �� ������ ���´�
			float theta2 = atan2(pt2.y - P.y, pt2.x - P.x); // atan2�� atan�� tan������ ����Ͽ� opencv���� ���ǻ� ���� �Լ���
			theta += (theta2 - theta1);// ������ ��� ������ �Ǿ�� src�κ��� ȸ���� ������ theta�� ����Ͽ� �������� ��ȯ�� �����ϴ�

			// Ȯ�����
			float r1 = sqrt((pt1.x - P.x) * (pt1.x - P.x) + (pt1.y - P.y) * (pt1.y - P.y)); // pt1�� �Ǻ� ������ �Ÿ�
			float r2 = sqrt((pt2.x - P.x) * (pt2.x - P.x) + (pt2.y - P.y) * (pt2.y - P.y)); // pt2�� �Ǻ� ������ �Ÿ�
			scale *= (r2 / r1);

			pt1 = pt2; // �̰� �����ָ� �巡�� �ÿ� pt1�� ó�����¿� �����ǰ� pt2�� ���������� Ŀ���Ƿ� �׸��� ȮȮ Ŀ����.
					   // pt1�� ���� ��ġ�� pt2�� �ʱ�ȭ�� �������ν� �� ���콺�� �̵��ϴ� ��ŭ���� �����ǵ��� �Ѵ�
		}
		if (draging == 2) { // shift + L��ư Ŭ�� �� �巡�� �� ��

			CvPoint pt2 = cvPoint(x, y); // ���콺�� �巡���� ���� ��ǥ�� pt2�� ����

			tx += (pt2.x - pt1.x); // ���콺�� Ŭ���� ��ġ���� ���� ��ġ������ x, y��ǥ ��ȭ����ŭ ������ �̵��ؾ� �Ѵ�.
			ty += (pt2.y - pt1.y);
			P.x += (pt2.x - pt1.x); // �Ǻ��� �Բ� �̵��ؾ� �Ѵ�
			P.y += (pt2.y - pt1.y);

			pt1 = pt2;
		}

		// TM1�� �Ǻ���ǥ�� �������� �̵���Ű�� ���
		// TM2�� �Ǻ���ǥ�� �ٽ� ����ġ�ϴ� ���

		setRotateMatrix(RM, -theta); // ������ ȸ���ϴ� IM��� ����
		setScaleMatrix(SM, 1.0f / scale, 1.0f / scale);
		setTranslateMatrix(TM1, P.x, P.y);
		setTranslateMatrix(TM2, -P.x, -P.y);
		setTranslateMatrix(TM, -tx, -ty);

		// ��� ���ϴ� ����
		//	 M = TM2 RM SM TM TM1 rem ������
		//	 IM = rem TM1 TM SM RM TM2 ������

		setMultiplyMatrix(temp1, rem, TM1);
		setMultiplyMatrix(temp2, TM, SM);
		setMultiplyMatrix(temp3, RM, TM2);
		setMultiplyMatrix(temp4, temp1, temp2);
		setMultiplyMatrix(IM, temp4, temp3);

		applyAffineTransform(src, dst, IM); // ��ȯ��� ����

		cvCopy(dst, buf);
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1); // �Ǻ��� �׷���

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
	P = cvPoint(cx, cy); // �Ǻ��� �ʱⰪ�� �̹����� �߽����� ����

	cvCopy(src, dst);

	// ��ȯ��ĵ��� �⺻��ķ� �ʱ�ȭ
	// �ʱ갪�� �⺻��ķ� �����ָ� shift�� Lbutton �� �� ó�� Ŭ�� �ÿ� multiplyMatrix�� �̻��� ����� ���� ��ȯ�� ������������ �ȴ�.
	setIdentityMatrix(IM);
	setIdentityMatrix(rem);
	setIdentityMatrix(RM);
	setIdentityMatrix(SM);
	setIdentityMatrix(TM);
	setIdentityMatrix(TM1);
	setIdentityMatrix(TM2);

	cvShowImage("dst", dst); // ó�� ȭ�鿣 �Ǻ��� �׷����� ���� �̹����� ���
	cvSetMouseCallback("dst", myMouse); // dst�̹������� myMouse�Լ� ����
	cvWaitKey();

	return 0;
}