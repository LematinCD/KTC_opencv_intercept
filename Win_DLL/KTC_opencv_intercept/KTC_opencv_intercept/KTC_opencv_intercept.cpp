// KTC_opencv_intercept.cpp : ���� DLL Ӧ�ó���ĵ���������
//
#include "stdafx.h"
#include <iostream>
#include<opencv2/objdetect/objdetect.hpp>  
#include<opencv2/highgui/highgui.hpp>  
#include<opencv2/imgproc/imgproc.hpp> 
#include"KTC_opencv_intercept.h"  //�����Ӧ���½����Ǹ�ͷ�ļ�

using namespace std;
using namespace cv;

#define MIN_POINT_X 0
#define MIN_POINT_Y 0

typedef struct Test_Opencv
{
	Mat srcPic; //Դͼ
	Mat greyPic;//�Ҷ�ͼ
	/*Mat *polyPic;*/
	int max_point_x_user;
	int max_point_y_user;
	float k;
	Point2f *tmp;
	int *index_first_buf;
	int *index_third_buf;
	int *size;
}test;

static int find_ch(char *origin_str, int str_size, int ch)
{
	char *p = origin_str;
	int count = 0;
	while ((*p++ != '\0') && (count<str_size)){
		count++;
		if (*p == ch)
			return count;
	}
	return 0;
}
static void my_insert(char *str, char *pch, int pos) {
	int len = strlen(str);
	int nlen = strlen(pch);
	for (int i = len - 1; i >= pos; --i) {
		*(str + i + nlen) = *(str + i);
	}
	for (int n = 0; n < nlen; n++)
		*(str + pos + n) = *(pch++);
	*(str + len + nlen) = 0;
}

static bool find_first_third_point(test *tt, int threshold_value)
{
	//5.תΪ��ֵͼƬ
	Mat binPic;
	if (threshold_value == NULL){
		threshold_value = 160;
	}
	threshold(tt->greyPic, binPic, threshold_value, 255, THRESH_BINARY);    //��ֵ��Ϊ��ֵͼƬ

	//6.����Canny��Ե���
	Mat cannyPic;
	double cannyThr = 200, FACTOR = 2.5;
	Canny(binPic, cannyPic, cannyThr, cannyThr*FACTOR);    //Canny��Ե���

	//7.��ȡ����
	vector<vector<Point> > contours;    //��������
	vector<Vec4i> hierarchy;
	findContours(cannyPic, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);    //��ȡ����

	//8.��ȡ��������������ö���ν�������Χ
	vector<vector<Point> > polyContours(contours.size());
	int maxArea = 0;
	for (int index = 0; index < contours.size(); index++){
		if (contourArea(contours[index]) > contourArea(contours[maxArea]))
			maxArea = index;
		approxPolyDP(contours[index], polyContours[index], 8, true);
	}
	//Mat polyPic = Mat::zeros((tt->srcPic).size(), CV_8UC3);
	//drawContours(polyPic, polyContours, maxArea, Scalar(0, 0, 255/*rand() & 255, rand() & 255, rand() & 255*/), 2);
	//*(tt->polyPic) = polyPic;
	//9.Ѱ��͹��
	vector<int>  hull;
	convexHull(polyContours[maxArea], hull, false);    //����������͹�� 
	for (int i = 0; i<hull.size(); i++){
		tt->tmp[i] = polyContours[maxArea][i];
	}

	for (int i = 0; i < hull.size(); i++){
		for (int j = 0; j< hull.size() - i - 1; j++){
			if (tt->tmp[j].x * tt->tmp[j].y > tt->tmp[j + 1].x * tt->tmp[j + 1].y){
				swap(tt->tmp[j], tt->tmp[j + 1]);
			}
		}
	}

	/*for(int i = 0;i < hull.size();i++){
	cout<<"tmp["<<i<<"]"<<tt->tmp[i]<<endl;
	}*/

	bool first_point_exist_flag = 0, third_point_exist_flag = 0;

	if ((tt->k >= 1) || (tt->k <= 0)){
		tt->k = 0.8;
	}
	if ((tt->max_point_x_user <= 0) || (tt->max_point_x_user > 4024)){
		tt->max_point_x_user = 4024;
	}
	if ((tt->max_point_y_user <= 0) || (tt->max_point_y_user > 3036)){
		tt->max_point_y_user = 3036;
	}
	/*printf("tt->k:%f --- tt->max_point_x_user:%d---tt->max_point_y_user:%d\n", tt->k,tt->max_point_x_user,tt->max_point_y_user);*/
	int count_1 = 0, count_3 = 0;
	for (int i = 0; i < hull.size(); i++){
		if ((tt->tmp[i].x  > MIN_POINT_X) && (tt->tmp[i].y > MIN_POINT_Y) && (tt->tmp[i].x * 1.0 < (tt->max_point_x_user * (1 - tt->k))) && (tt->tmp[i].y* 1.0 < (tt->max_point_y_user * (1 - tt->k)))){
			/*printf("first point exist!!!\n");
			cout << "1---tmp[" << i << "]" << tt->tmp[i] << endl;*/
			(tt->index_first_buf)[++count_1] = i;
			first_point_exist_flag = 1;
		}

		if ((tt->tmp[i].x * 1.0 > tt->max_point_x_user * tt->k) && (tt->tmp[i].y *1.0 > tt->max_point_y_user * tt->k) && (tt->tmp[i].x < tt->max_point_x_user) && (tt->tmp[i].y < tt->max_point_y_user)){
			/*printf("third point exist!!!\n");
			cout << "3---tmp[" << i << "]" << tt->tmp[i] << endl;*/
			(tt->index_third_buf)[++count_3] = i;
			third_point_exist_flag = 1;
		}
	}

	(tt->index_first_buf)[0] = count_1;
	(tt->index_third_buf)[0] = count_3;
	if ((first_point_exist_flag) && (third_point_exist_flag)){
		*(tt->size) = (int)hull.size();
		return true;
	}
	else
		return false;
}

int __stdcall KTC_image_extract(char *input_file_path,
								int deviation, 
								int threshold_thresh, 
								float k, 
								char *output_file_ptr, 
								int *output_count)
{
	//1.����Դͼ�� 
	//Mat inImg = imread("C:\\Image_20181219180711651.jpg");
	Mat inImg = imread(input_file_path);
	if (inImg.empty()){
		/*printf("ͼƬ������!\n");*/
		return 0;
	}
	//printf("inImg.cols:%d,inImg.row:%d\n", inImg.cols, inImg.rows);
	//namedWindow("input image", 0);
	//imshow("input image", inImg);
	Mat srcPic = inImg.clone();//��Ҫֱ����Դͼ���Ͻ��в���

	//2.Ԥ���� ��С4��	
	//Mat shrinkedPic; 
	//pyrDown(srcPic,shrinkedPic);        //��С�ߴ� �ӿ������ٶ�
	//pyrDown(shrinkedPic,shrinkedPic);
	//namedWindow("output image shrinkedPic",0); 
	//imshow("output image shrinkedPic", shrinkedPic);	


	//3.ת��Ϊ�Ҷ�ͼ
	Mat greyPic;
	cvtColor(srcPic, greyPic, COLOR_BGR2GRAY);

	//4.��ֵ�˲�
	//medianBlur(greyPic, greyPic, 7); //min > 1
	int ksize1 = 33;
	int ksize2 = 33;
	double sigma1 = 10.0;
	double sigma2 = 20.0;
	GaussianBlur(greyPic, greyPic, Size(ksize1, ksize2), sigma1, sigma2);
	/*namedWindow("output image greyPic", 0);
	imshow("output image greyPic", greyPic);*/

	Mat polyPic = Mat::zeros(srcPic.size(), CV_8UC3);
	Point2f tmp[100];
	int size = 0;
	int index_first_buf[100], index_third_buf[100];
	test tt;
	tt.srcPic = srcPic;
	tt.greyPic = greyPic;
	/*tt.polyPic = &polyPic;*/
	tt.max_point_x_user = inImg.cols;
	tt.max_point_y_user = inImg.rows;
	tt.tmp = tmp;
	tt.index_first_buf = index_first_buf;
	tt.index_third_buf = index_third_buf;
	tt.k = k;
	tt.size = &size;
	int fir_thd_flg = false;
	if ((threshold_thresh < 0) || (threshold_thresh > 180)){
		threshold_thresh = 150;
	}
	/*printf("threshold_thresh:%d\n", threshold_thresh);*/
	for (int threshold_value = 80; threshold_value < threshold_thresh; threshold_value++){
		if (true == find_first_third_point(&tt, threshold_value)){
			/*printf("threshold_value:%d\n", threshold_value);*/
			fir_thd_flg = true;
			break;
		}
	}
	if (fir_thd_flg == false)
		return -1;
	/*for (int i = 0; i < size; ++i){
		circle(polyPic, tmp[i], 10, Scalar(rand() & 255, rand() & 255, rand() & 255), 3);
	}
	addWeighted(polyPic, 1, srcPic, 1, 0, srcPic);
	namedWindow("output image srcPic2", 0);
	imshow("output image srcPic2", polyPic);
	printf("tt.index_first_buf[0]:%d---tt.index_third_buf[0]:%d\n", tt.index_first_buf[0], tt.index_third_buf[0]);
	for (int i = 0; i < size; ++i){
		cout << "tmp[" << i << "]" << tmp[i] << endl;
	}*/
	
	/**
		�������Ϻ�����������
	*/
	Point2f final_1, final_3;
	int count_1 = tt.index_first_buf[0];
	int count_3 = tt.index_third_buf[0];
	if (tt.index_first_buf[0] > 1){
		Point2f *tmp_1_x = new Point2f[count_1]();
		Point2f *tmp_1_y = new Point2f[count_1]();
		for (int i = 0; i< count_1; i++){
			tmp_1_x[i] = tmp[tt.index_first_buf[i + 1]];
			tmp_1_y[i] = tmp[tt.index_first_buf[i + 1]];
		}
		for (int i = 0; i < count_1; i++){
			for (int j = 0; j< count_1 - i - 1; j++){
				if (tmp_1_x[j].x >tmp_1_x[j + 1].x)
					swap(tmp_1_x[j], tmp_1_x[j + 1]);
			}
		}
		for (int i = 0; i < count_1; i++){
			for (int j = 0; j< count_1 - i - 1; j++){
				if (tmp_1_y[j].y >tmp_1_y[j + 1].y)
					swap(tmp_1_y[j], tmp_1_y[j + 1]);
			}
		}

		final_1.x = tmp_1_x[0].x;
		final_1.y = tmp_1_y[0].y;

		delete(tmp_1_x);
		delete(tmp_1_y);
	}
	else{
		final_1 = tmp[0];
	}

	if (tt.index_third_buf[0] > 1){
		Point2f *tmp_3_x = new Point2f[tt.index_third_buf[0]]();
		Point2f *tmp_3_y = new Point2f[tt.index_third_buf[0]]();

		for (int i = 0; i< count_3; i++){
			tmp_3_x[i] = tmp[tt.index_third_buf[i + 1]];
			tmp_3_y[i] = tmp[tt.index_third_buf[i + 1]];
		}

		for (int i = 0; i < count_3; i++){
			for (int j = 0; j< count_3 - i - 1; j++){
				if (tmp_3_x[j].x >tmp_3_x[j + 1].x)
					swap(tmp_3_x[j], tmp_3_x[j + 1]);
			}
		}
		for (int i = 0; i < count_3; i++){
			for (int j = 0; j< count_3 - i - 1; j++){
				if (tmp_3_y[j].y >tmp_3_y[j + 1].y)
					swap(tmp_3_y[j], tmp_3_y[j + 1]);
			}
		}
		/*for(int i = 0;i< count_3;i++){
		cout<<"tmp_3_x["<<i<<"]"<<tmp_3_x[i]<<endl;
		cout<<"tmp_3_y["<<i<<"]"<<tmp_3_y[i]<<endl;
		}*/
		final_3.x = tmp_3_x[count_3 - 1].x;
		final_3.y = tmp_3_y[count_3 - 1].y;

		delete(tmp_3_x);
		delete(tmp_3_y);
	}
	else{
		final_3 = tmp[size - 1];
	}

	/**
		�������Ϻ�����������
	*/
	Point2f *tmp_2 = new Point2f[size]();
	Point2f *tmp_4 = new Point2f[size]();
	int count_2 = 0, count_4 = 0;
	for (int i = 1; i<size - 1; i++){
		if (((tmp[i].y<(tmp[0].y + deviation)) && (tmp[i].y>(tmp[0].y - deviation))) && ((tmp[i].x<(tmp[size - 1].x + deviation)) && (tmp[i].x>(tmp[size - 1].x - deviation)))){
			tmp_2[count_2++] = tmp[i];
		}
		if (((tmp[i].x<(tmp[0].x + deviation)) && (tmp[i].x>(tmp[0].x - deviation))) && ((tmp[i].y<(tmp[size - 1].y + deviation)) && (tmp[i].y>(tmp[size - 1].y - deviation)))){
			tmp_4[count_4++] = tmp[i];
		}
	}
	/*printf("##########count_2:%d---count_4:%d\n",count_2,count_4);
	for (int i = 0; i < count_2; i++){
		cout << "tmp_2[" << i << "]" << tmp_2[i] << endl;
	}
	for (int i = 0; i < count_4; i++){
		cout << "tmp_4[" << i << "]" << tmp_4[i] << endl;
	}*/

	Point2f final_2, final_4;
	bool first_point_exist_flag = 0, third_point_exist_flag = 0;
	if (count_2 > 1){
		//Point2f tmp_2_x[count_2],tmp_2_y[count_2];
		Point2f *tmp_2_x = new Point2f[count_2]();
		Point2f *tmp_2_y = new Point2f[count_2]();

		for (int i = 0; i < count_2; i++){
			tmp_2_x[i] = tmp_2[i];
			tmp_2_y[i] = tmp_2[i];
		}
		for (int i = 0; i < count_2; i++){
			for (int j = 0; j< count_2 - i - 1; j++){
				if (tmp_2_x[j].x >tmp_2_x[j + 1].x)
					swap(tmp_2_x[j], tmp_2_x[j + 1]);
			}
		}
		for (int i = 0; i < count_2; i++){
			for (int j = 0; j< count_2 - i - 1; j++){
				if (tmp_2_y[j].y >tmp_2_y[j + 1].y)
					swap(tmp_2_y[j], tmp_2_y[j + 1]);
			}
		}

		final_2.x = tmp_2_x[count_2 - 1].x;
		final_2.y = tmp_2_y[0].y;
		first_point_exist_flag = 1;
		delete(tmp_2_x);
		delete(tmp_2_y);

	}
	else if (count_2 == 1){
		final_2 = tmp_2[0];
		first_point_exist_flag = 1;
	}
	else if (count_2 == 0){
		final_2.x = tmp[size - 1].x;
		final_2.y = tmp[0].y;
		first_point_exist_flag = 1;
	}

	if (count_4 > 1){
		//Point2f tmp_4_x[count_4],tmp_4_y[count_4];
		Point2f *tmp_4_x = new Point2f[count_4]();
		Point2f *tmp_4_y = new Point2f[count_4]();
		for (int i = 0; i < count_4; i++){
			tmp_4_x[i] = tmp_4[i];
			tmp_4_y[i] = tmp_4[i];
		}

		for (int i = 0; i < count_4; i++){
			for (int j = 0; j< count_4 - i - 1; j++){
				if (tmp_4_x[j].x >tmp_4_x[j + 1].x)
					swap(tmp_4_x[j], tmp_4_x[j + 1]);
			}
		}
		for (int i = 0; i < count_4; i++){
			for (int j = 0; j< count_4 - i - 1; j++){
				if (tmp_4_y[j].y >tmp_4_y[j + 1].y)
					swap(tmp_4_y[j], tmp_4_y[j + 1]);
			}
		}
		final_4.x = tmp_4_x[0].x;
		final_4.y = tmp_4_y[count_4 - 1].y;
		third_point_exist_flag = 1;
		delete(tmp_4_x);
		delete(tmp_4_y);

	}
	else if (count_4 == 1){
		final_4 = tmp_4[0];
		third_point_exist_flag = 1;
	}
	else if (count_4 == 0){
		final_4.x = tmp[0].x;
		final_4.y = tmp[size - 1].y;
		third_point_exist_flag = 1;
	}

	if ((first_point_exist_flag == 0) || (third_point_exist_flag == 0)){
		//printf("***********************\n");
		return -2;
	}
	//10.ͶӰ�任
	Point2f srcPoints_man[4], srcPoints[4], dstPoints[4];
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(srcPic.cols, 0);
	dstPoints[2] = Point2f(srcPic.cols, srcPic.rows);
	dstPoints[3] = Point2f(0, srcPic.rows);

	srcPoints_man[0] = final_1;
	srcPoints_man[1] = final_2;
	srcPoints_man[2] = final_3;
	srcPoints_man[3] = final_4;
	/*cout << "srcPoints_man[0]: " << srcPoints_man[0] << endl;
	cout << "srcPoints_man[1]: " << srcPoints_man[1] << endl;
	cout << "srcPoints_man[2]: " << srcPoints_man[2] << endl;
	cout << "srcPoints_man[3]: " << srcPoints_man[3] << endl;*/

	Mat transMat = getPerspectiveTransform(srcPoints_man, dstPoints);    //�õ��任����
	Mat outPic;
	warpPerspective(srcPic, outPic, transMat, srcPic.size());    //��������任
	//namedWindow("output image outPic", 0);
	//imshow("output image outPic", outPic);

	char *str_tmp = "_extracted";
	char *output_file_path = new char[strlen(input_file_path) + strlen(str_tmp) + 1];
	int count = find_ch(input_file_path, strlen(input_file_path) + 1, '.');
	strcpy_s(output_file_path, strlen(input_file_path) + 1, input_file_path);
	my_insert(output_file_path, str_tmp, count);
	imwrite(output_file_path, outPic);
	strcpy_s(output_file_ptr, strlen(output_file_path) + 1, output_file_path);
	*output_count = strlen(output_file_path);

	delete(output_file_path);
	delete(tmp_2);
	delete(tmp_4);

	//waitKey(0);
	return 1;
}