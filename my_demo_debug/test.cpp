#include<opencv2/objdetect/objdetect.hpp>  
#include<opencv2/highgui/highgui.hpp>  
#include<opencv2/imgproc/imgproc.hpp>  
#include <iostream>
#include<stdio.h>

using namespace cv;
using namespace std;



#define MIN_POINT_X 0
#define MIN_POINT_Y 0
//#define MAX_POINT_X 3840
//#define MAX_POINT_Y 2160
#define MAX_POINT_X 4024
#define MAX_POINT_Y 3036



typedef struct Test_Opencv
{
	Mat srcPic;
	Mat greyPic;
	Mat *polyPic;
	float k;
	Point2f *tmp;
	int *index_first_buf;
	int *index_third_buf;
	int *size;
}test;

static bool find_first_third_point(test *tt,int threshold_value)
{
	//5.转为二值图片
	Mat binPic;
	if (threshold_value == NULL){
		threshold_value = 160;
	}
	threshold(tt->greyPic, binPic, threshold_value, 255, THRESH_BINARY);    //阈值化为二值图片

	//6.进行Canny边缘检测
	Mat cannyPic;
	double cannyThr = 200, FACTOR = 2.5;
	Canny(binPic, cannyPic, cannyThr, cannyThr*FACTOR);    //Canny边缘检测
	
	//7.提取轮廓
	vector<vector<Point> > contours;    //储存轮廓
	vector<Vec4i> hierarchy;
	findContours(cannyPic, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);    //获取轮廓

	//8.提取面积最大的轮廓并用多边形将轮廓包围
	vector<vector<Point> > polyContours(contours.size());
	int maxArea = 0;
	for (int index = 0; index < contours.size(); index++){        
		if (contourArea(contours[index]) > contourArea(contours[maxArea]))
        	maxArea = index;        
        approxPolyDP(contours[index], polyContours[index], 8, true);
    }
	Mat polyPic = Mat::zeros((tt->srcPic).size(), CV_8UC3);
	//Mat polyPic = Mat::zeros((tt->srcPic).size(), CV_8UC3);
	//drawContours(polyPic, polyContours, maxArea, Scalar(0,0,255/*rand() & 255, rand() & 255, rand() & 255*/), 2);
	//for (int i = 0; i < size; ++i){
    //    circle(polyPic, tmp[i], 10, Scalar(rand() & 255, rand() & 255, rand() & 255), 3);
    //}	
	//namedWindow("output image polyPic",0); 
	//imshow("output image polyPic", polyPic);
	*(tt->polyPic) = polyPic;
	//9.寻找凸包
	vector<int>  hull;
	convexHull(polyContours[maxArea], hull, false);    //检测该轮廓的凸包 

	
	drawContours(polyPic, polyContours, maxArea, Scalar(0,0,255/*rand() & 255, rand() & 255, rand() & 255*/), 2);
	for (int i = 0; i < hull.size(); ++i){
        circle(polyPic, polyContours[maxArea][i], 10, Scalar(rand() & 255, rand() & 255, rand() & 255), 3);
    }	
	namedWindow("output image polyPic",0); 
	imshow("output image polyPic", polyPic);
	for(int i =0;i<hull.size();i++){
		tt->tmp[i] = polyContours[maxArea][i];
	}

	for (int i = 0; i < hull.size(); i++){
		for(int j = 0;j< hull.size() - i -1;j++){
			if(tt->tmp[j].x * tt->tmp[j].y > tt->tmp[j+1].x * tt->tmp[j+1].y){
				swap(tt->tmp[j],tt->tmp[j+1]);
			}
		}
	}
	
	/*for(int i = 0;i < hull.size();i++){
		cout<<"tmp["<<i<<"]"<<tt->tmp[i]<<endl;
	}*/

	bool first_point_exist_flag = 0,third_point_exist_flag = 0;
	
	if((tt->k >= 1) ||(tt->k <= 0)){
		tt->k = 0.8;
	}
	//printf("tt->k:%f\n",tt->k);
	int count_1=0,count_3=0;
	for(int i= 0;i < hull.size();i++){
		if((tt->tmp[i].x  > MIN_POINT_X)&&(tt->tmp[i].y > MIN_POINT_Y)&&(tt->tmp[i].x * 1.0 < (MAX_POINT_X * (1-tt->k)))&&(tt->tmp[i].y* 1.0 < (MAX_POINT_Y * (1-tt->k)))){
			printf("first point exist!!!\n");
			cout<<"1---tmp["<<i<<"]"<<tt->tmp[i]<<endl;
			(tt->index_first_buf)[++count_1] = i;			
			first_point_exist_flag = 1;
		}
		
		if((tt->tmp[i].x * 1.0 > MAX_POINT_X * tt->k)&&(tt->tmp[i].y *1.0 > MAX_POINT_Y * tt->k)&&(tt->tmp[i].x < MAX_POINT_X)&&(tt->tmp[i].y < MAX_POINT_Y)){
			printf("third point exist!!!\n");
			cout<<"3---tmp["<<i<<"]"<<tt->tmp[i]<<endl;
			(tt->index_third_buf)[++count_3] = i;	
			third_point_exist_flag = 1;
		}
	}
	
	(tt->index_first_buf)[0] = count_1;
	(tt->index_third_buf)[0] = count_3;
	if((first_point_exist_flag)&&(third_point_exist_flag)){
		*(tt->size) = (int)hull.size();
		return true;
	}
	else
		return false;
}

int main(void) 
{ 
	//1.读入源图像 
	Mat inImg = imread("Image_20190103151643739.jpg");
	namedWindow("input image",0); 
	imshow("input image", inImg); 
	Mat srcPic = inImg.clone();//不要直接在源图像上进行操作
	
	//2.预处理 缩小4倍	
	//Mat shrinkedPic; 
	//pyrDown(srcPic,shrinkedPic);        //减小尺寸 加快运算速度
	//pyrDown(shrinkedPic,shrinkedPic);
	//namedWindow("output image shrinkedPic",0); 
	//imshow("output image shrinkedPic", shrinkedPic);	
	
	
	//3.转化为灰度图
	Mat greyPic;
	cvtColor(srcPic, greyPic, COLOR_BGR2GRAY);    
	
	//4.中值滤波 
	//medianBlur(greyPic, greyPic, 3); //min > 1
	int ksize1 = 33;
    int ksize2 = 33;
    double sigma1 = 10.0;
    double sigma2 = 20.0;
	GaussianBlur(greyPic,greyPic,Size(ksize1,ksize2), sigma1, sigma2);
	

	Mat polyPic = Mat::zeros(srcPic.size(), CV_8UC3);
	Point2f tmp[100];
	int size = 0;
	int index_first_buf[100],index_third_buf[100];
	test tt;
	tt.srcPic = srcPic;
	tt.greyPic = greyPic;
	tt.polyPic = &polyPic;
	tt.tmp = tmp;
	tt.index_first_buf = index_first_buf;
	tt.index_third_buf = index_third_buf;
	tt.k = 0.7;
	tt.size = &size;
	for(int threshold_value = 1;threshold_value < 180;threshold_value++){	
		//if(true==find_first_third_point(srcPic,greyPic,threshold_value,0.9,tmp,&size)){
		printf("threshold_value:%d\n",threshold_value);
		if(true==find_first_third_point(&tt,threshold_value)){
			break;
		}
		waitKey(100);
	}

	
	for (int i = 0; i < size; ++i){
        circle(polyPic, tmp[i], 10, Scalar(rand() & 255, rand() & 255, rand() & 255), 3);
		//cout<<"polyContours["<<maxArea<<"]"<<"["<<i<<"]: "<<polyContours[maxArea][i]<<endl;
    }

	for (int i = 0; i < size; ++i){
        cout<<"tmp["<<i<<"]"<<tmp[i]<<endl;
    }
	addWeighted(polyPic, 1, srcPic, 1, 0, srcPic);

	namedWindow("output image srcPic2",0); 
	imshow("output image srcPic2", polyPic);
	printf("tt.index_first_buf[0]:%d---tt.index_third_buf[0]:%d\n",tt.index_first_buf[0],tt.index_third_buf[0]);
	//waitKey(0);
	Point2f final_1,final_3;
	int count_1 = tt.index_first_buf[0];
	int count_3 = tt.index_third_buf[0];
	printf("count_1:%d---count_3:%d\n",count_1,count_3);
	if(tt.index_first_buf[0] > 1){
		printf("lalala\n");
		Point2f *tmp_1_x = new Point2f[count_1]();
		Point2f *tmp_1_y = new Point2f[count_1]();
		for(int i = 0;i< count_1;i++){
			tmp_1_x[i] = tmp[tt.index_first_buf[i+1]];
			tmp_1_y[i] = tmp[tt.index_first_buf[i+1]];
		}
	
		for (int i = 0; i < count_1; i++){
			for(int j = 0;j< count_1 - i -1;j++){
				if(tmp_1_x[j].x >tmp_1_x[j+1].x)
					swap(tmp_1_x[j],tmp_1_x[j+1]);
			}
		}

		for (int i = 0; i < count_1; i++){
			for(int j = 0;j< count_1 - i -1;j++){
				if(tmp_1_y[j].y >tmp_1_y[j+1].y)
					swap(tmp_1_y[j],tmp_1_y[j+1]);
			}
		}

		final_1.x = tmp_1_x[0].x;
		final_1.y = tmp_1_y[0].y; 


		delete(tmp_1_x);
		delete(tmp_1_y);
	}else{
		final_1 = tmp[0];
	}

	cout<<"final_1"<<final_1<<endl;
	if(tt.index_third_buf[0] > 1){
		Point2f *tmp_3_x = new Point2f[tt.index_third_buf[0]]();
		Point2f *tmp_3_y = new Point2f[tt.index_third_buf[0]]();

		for(int i = 0;i< count_3;i++){
			tmp_3_x[i] = tmp[tt.index_third_buf[i+1]];
			tmp_3_y[i] = tmp[tt.index_third_buf[i+1]];
		}

		for (int i = 0; i < count_3; i++){
			for(int j = 0;j< count_3 - i -1;j++){
				if(tmp_3_x[j].x >tmp_3_x[j+1].x)
					swap(tmp_3_x[j],tmp_3_x[j+1]);
			}
		}

		for (int i = 0; i < count_3; i++){
			for(int j = 0;j< count_3 - i -1;j++){
				if(tmp_3_y[j].y >tmp_3_y[j+1].y)
					swap(tmp_3_y[j],tmp_3_y[j+1]);
			}
		}

		/*for(int i = 0;i< count_3;i++){
			cout<<"tmp_3_x["<<i<<"]"<<tmp_3_x[i]<<endl;
			cout<<"tmp_3_y["<<i<<"]"<<tmp_3_y[i]<<endl;
		}*/
		final_3.x = tmp_3_x[count_3-1].x;
		final_3.y = tmp_3_y[count_3-1].y; 
	
		delete(tmp_3_x);
		delete(tmp_3_y);
	}else{
		final_3 = tmp[size-1];
	}
	
	//Point2f tmp_2[size],tmp_4[size];
	Point2f *tmp_2 = new Point2f[size]();
	Point2f *tmp_4 = new Point2f[size]();
	int count_2 = 0,count_4 = 0;
	int deviation = 200;
	for(int i = 1;i<size-1;i++){
		if(((tmp[i].y<(tmp[0].y+deviation)) && (tmp[i].y>(tmp[0].y-deviation))) && ((tmp[i].x<(tmp[size-1].x+deviation)) && (tmp[i].x>(tmp[size-1].x-deviation)))){	
				tmp_2[count_2++] = tmp[i];	
		}	
		if(((tmp[i].x<(tmp[0].x+deviation)) && (tmp[i].x>(tmp[0].x-deviation))) && ((tmp[i].y<(tmp[size-1].y+deviation)) && (tmp[i].y>(tmp[size-1].y-deviation)))){	
				tmp_4[count_4++] = tmp[i];	
		}
	}
	

	Point2f final_2,final_4;
	if(count_2 > 1){
		//Point2f tmp_2_x[count_2],tmp_2_y[count_2];
		Point2f *tmp_2_x = new Point2f[count_2]();
		Point2f *tmp_2_y = new Point2f[count_2]();
		
		for(int i = 0; i < count_2;i++){
			tmp_2_x[i] = tmp_2[i];
			tmp_2_y[i] = tmp_2[i];
		}

		
		for (int i = 0; i < count_2; i++){
			for(int j = 0;j< count_2 - i -1;j++){
				if(tmp_2_x[j].x >tmp_2_x[j+1].x)
					swap(tmp_2_x[j],tmp_2_x[j+1]);
			}
		}

		for (int i = 0; i < count_2; i++){
			for(int j = 0;j< count_2 - i -1;j++){
				if(tmp_2_y[j].y >tmp_2_y[j+1].y)
					swap(tmp_2_y[j],tmp_2_y[j+1]);
			}
		}

		final_2.x = tmp_2_x[count_2-1].x;
		final_2.y = tmp_2_y[0].y;
		delete(tmp_2_x);
		delete(tmp_2_y);
				
	}else if(count_2 == 1){
		final_2 = tmp_2[0]; 
	}



	if(count_4 > 1){
		//Point2f tmp_4_x[count_4],tmp_4_y[count_4];
		Point2f *tmp_4_x = new Point2f[count_4]();
		Point2f *tmp_4_y = new Point2f[count_4]();
		for(int i = 0; i < count_4;i++){
			tmp_4_x[i] = tmp_4[i];
			tmp_4_y[i] = tmp_4[i];
		}
		
		for (int i = 0; i < count_4; i++){
			for(int j = 0;j< count_4 - i -1;j++){
				if(tmp_4_x[j].x >tmp_4_x[j+1].x)
					swap(tmp_4_x[j],tmp_4_x[j+1]);
			}
		}
		for (int i = 0; i < count_4; i++){
			for(int j = 0;j< count_4 - i -1;j++){
				if(tmp_4_y[j].y >tmp_4_y[j+1].y)
					swap(tmp_4_y[j],tmp_4_y[j+1]);
			}
		}
		final_4.x = tmp_4_x[0].x;
		final_4.y = tmp_4_y[count_4-1].y;
		delete(tmp_4_x);
		delete(tmp_4_y);
				
	}else if(count_4 == 1){
		final_4 = tmp_4[0]; 
	}
	
	

	//10.投影变换
	Point2f srcPoints_man[4], srcPoints[4],dstPoints[4];
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(srcPic.cols, 0);
	dstPoints[2] = Point2f(srcPic.cols, srcPic.rows);
	dstPoints[3] = Point2f(0, srcPic.rows);
	
	srcPoints_man[0] = final_1;
	//srcPoints_man[1] = final_2;
	srcPoints_man[1] = Point2f(3800, 579);
	srcPoints_man[2] = final_3;
	srcPoints_man[3] = final_4;
	cout << "srcPoints_man[0]: "<< srcPoints_man[0] << endl;
	cout << "srcPoints_man[1]: "<< srcPoints_man[1] << endl;
	cout << "srcPoints_man[2]: "<< srcPoints_man[2] << endl;
	cout << "srcPoints_man[3]: "<< srcPoints_man[3] << endl;
	
	Mat transMat = getPerspectiveTransform(srcPoints_man, dstPoints);    //得到变换矩阵
	Mat outPic;
	warpPerspective(srcPic, outPic, transMat, srcPic.size());    //进行坐标变换
	namedWindow("output image outPic",0); 
	imshow("output image outPic", outPic);

	imwrite("output_image_outPic.jpg",outPic);
	delete(tmp_2);
	delete(tmp_4);
	waitKey(0);    
	return 0; 
}

