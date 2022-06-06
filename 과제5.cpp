#include <opencv2/opencv.hpp>

// 1. shift�� �Ϲݵ巡�� ���Ḹ ���ָ� �ȴ�.
//		---> ���������� Ǯ�� ������ķ� �̾���δ�
//		---> drag 1, 2���� ��� ������ ���Խ�Ų��
// *** ��� ���������ʹ� ������ ����ְ� ����� �������� ���̴�.

// ������ �������� ���� ��� �Լ����� ����� ���� �����Ѵ�

// ���� ����� P �������� ��ȯ�ϰ� �� ��������� ���Ӱ� ���� P �������� �ٽ� ��ȯ�ؾ� �ϴµ�
// �̷��� ������ ��� �����͸� ����ϵ��� �ϸ� 
// �������� ���� P �������� �� �ѹ��� ��ȯ�� ���� ���̰� �ȴ�.

// �׷��� R��ư Ŭ�� �ÿ� src�� dst�� �������ָ�
// ���������� �߸��ų� ������ ǳȭ�� �Ͼ��.

// ���� R��ư Ŭ���ÿ� ���� ȸ���� ������ ��ȯ�� ����ϴ� ����� ���� �����ָ� ���?

// ��� ���� �Լ�
void copyMatrix(float IM[][3], float copy_IM[][3]) {
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			copy_IM[i][j] = IM[i][j];
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
void setRotateMatrix(float M[][3], float theta) {
	setIdentityMatrix(M);
	float rad = theta * 3.141592 / 180.0f; // degree�� radian���� ��ȯ

	M[0][0] = cos(rad);			M[0][1] = -sin(rad); // ȸ�����
	M[1][0] = sin(rad);			M[1][1] = cos(rad);
}

// ��ġ �̵� �Լ�
void setTranslateMatrix(float M[][3], float tx, float ty) {
	setIdentityMatrix(M);
	M[0][2] = tx; // �̵� ��� �������
	M[1][2] = ty;
}

// 3X3 ����� ���� ������ִ� �Լ�
void setMultiplyMatrix(float M[][3], float A[][3], float B[][3]) {
	// M = A*B
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++) {
			M[i][j] = 0; // 0���� �ʱ�ȭ
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

// �����̵�TM1 -> Ȯ�����M2 -> ȸ��M1 -> �����̵�TM -> ����ġ�̵�TM2

float M1[3][3];
float M2[3][3];
float TM[3][3];
float TM1[3][3];
float TM2[3][3];
float temp1[3][3];
float temp2[3][3];
float temp3[3][3];
float temp4[3][3];

float rem[3][3]; // ���� ȸ���࿡���� ��ȯ�� ����ϴ� ���

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
		P = cvPoint(x, y);
		

		theta = 0.0f; // �������� �ʱ�ȭ
		scale = 1.0f;
		tx = 0.f;
		ty = 0.f;


		copyMatrix(IM, rem); // IM�� rem�� ����

		cvCopy(dst, buf);
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1);
		cvShowImage("dst", buf);
	}
	if (event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) == CV_EVENT_FLAG_LBUTTON) { // �巡��

		if (draging == 1) { // �Ϲ� �巡�� �� ��
			CvPoint pt2 = cvPoint(x, y); // ���콺�� �巡���� ���� ��ǥ�� pt2�� ����
			// ȸ��
			float theta1 = atan2(pt1.y - P.y, pt1.x - P.x); // atan�� �̿��ϸ� ��ǥ�� ���� ������ ��������� ����
			float theta2 = atan2(pt2.y - P.y, pt2.x - P.x); // atan2�� ������ tan���� ������ opencv���� ���ǻ� ���� �Լ���
			// ������ degree�� �־�� �ϴµ� atan2���� ������ ���� radian�̶� ��ȯ�� �ʿ�
			theta += (theta2 - theta1) / 3.141592 * 180.0f; // ������ ��� ������ �Ǿ�� ���� �׸����� ���� ������ �����ϴ�

			// Ȯ�����
			float r1 = sqrt((pt1.x - P.x) * (pt1.x - P.x) + (pt1.y - P.y) * (pt1.y - P.y)); // pt1�� ȸ���� ������ �Ÿ�
			float r2 = sqrt((pt2.x - P.x) * (pt2.x - P.x) + (pt2.y - P.y) * (pt2.y - P.y)); // pt2�� ȸ���� ������ �Ÿ�
			scale *= (r2 / r1);

			

			// TM1�� �׸��� �߽��� �������� �̵���Ű�� ���
			// TM2�� �ٽ� �׸��� ����ġ�� �̵���Ű�� ���


			pt1 = pt2; // �̰� �����ָ� �巡�� �ÿ� pt1�� ó�����¿� �����ǰ� pt2�� ���������� Ŀ���Ƿ� �׸��� ȮȮ Ŀ����.
					   // pt1�� ���� ��ġ�� pt2�� �ʱ�ȭ�� ���־�� �Ѵ�.

		}
		if (draging == 2) { // shift + L��ư Ŭ�� �� �巡�� �� ��
			// ���⼭ �߿��Ѱ��� ������ ��ġ�� ����� �� ȸ����(P) ��ǥ�� �Բ� �������� �־�� �Ѵٴ� ���̴�.

			CvPoint pt2 = cvPoint(x, y); // ���콺�� �巡���� ���� ��ǥ�� pt2�� ����

			tx += (pt2.x - pt1.x); // ���콺�� Ŭ���� ��ġ���� d���� ��ġ������ x, y��ǥ ��ȭ����ŭ ������ �̵��ؾ� �Ѵ�.
			ty += (pt2.y - pt1.y);
			P.x += (pt2.x - pt1.x); // ȸ���൵ �Բ� �̵��ؾ� �Ѵ�
			P.y += (pt2.y - pt1.y);

			pt1 = pt2;
		}


		setRotateMatrix(M1, -theta); // ������ ȸ���ϴ� IM��� ����
		setScaleMatrix(M2, 1.0f / scale, 1.0f / scale);
		setTranslateMatrix(TM1, P.x, P.y);
		setTranslateMatrix(TM2, -P.x, -P.y);
		setTranslateMatrix(TM, -tx, -ty);

	/*	 M = TM2 M1 M2 TM TM1 rem
		 IM = rem TM1 TM M2 M1 TM2  */

		setMultiplyMatrix(temp1, rem, TM1);
		setMultiplyMatrix(temp2, TM, M2);
		setMultiplyMatrix(temp3, M1, TM2);
		setMultiplyMatrix(temp4, temp1, temp2);
		setMultiplyMatrix(IM, temp4, temp3);
		

		applyAffineTransform(src, dst, IM); // ��ȯ��� ����

		cvCopy(dst, buf);
		cvCircle(buf, P, 5, cvScalar(255, 0, 0), -1); // ȸ������ �׷���

		cvShowImage("dst", buf);
	}
}


int main() {

	cvCopy(src, dst);

	// �ʱ갪�� �⺻��ķ� �����ָ� shift�� Lbutton �� �� ó�� Ŭ�� �ÿ� multiplyMatrix�� �̻��� ����� ���� ��ȯ�� ������������ �ȴ�.
	setIdentityMatrix(IM);
	setIdentityMatrix(rem);
	setIdentityMatrix (M1);
	setIdentityMatrix (M2);
	setIdentityMatrix (TM);
	setIdentityMatrix (TM1);
	setIdentityMatrix (TM2);

	cvShowImage("dst", dst);
	cvSetMouseCallback("dst", myMouse); // dst�̹������� myMouse�Լ� ���� (�ϱ�) 
	cvWaitKey();

	return 0;
}