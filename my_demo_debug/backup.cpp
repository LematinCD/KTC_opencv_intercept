#include<opencv2/objdetect/objdetect.hpp>  
#include<opencv2/highgui/highgui.hpp>  
#include<opencv2/imgproc/imgproc.hpp>  
#include <iostream>
#include<stdio.h>

using namespace cv;
using namespace std;
//寻找最大外接轮廓
vector<Point> FindBigestContour(Mat src){
    int max_area_contour_idx = 0;
    double max_area = -1;
    vector<vector<Point> >contours;
    findContours(src,contours,RETR_LIST,CHAIN_APPROX_SIMPLE);
    //handle case if no contours are detected
    CV_Assert(0 != contours.size());
    for (uint i=0;i<contours.size();i++){
        double temp_area = contourArea(contours[i]);
        if (max_area < temp_area ){
            max_area_contour_idx = i;
            max_area = temp_area;
        }
    }
    return contours[max_area_contour_idx];
}

int main(void) 
{ 
	//1.读入源图像 
	Mat inImg = imread("IMG_0459.JPG");
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
	//medianBlur(greyPic, greyPic, 7); //min > 1
	int ksize1 = 33;
    int ksize2 = 33;
    double sigma1 = 10.0;
    double sigma2 = 20.0;
	GaussianBlur(greyPic,greyPic,Size(ksize1,ksize2), sigma1, sigma2);
	namedWindow("output image greyPic",0); 
	imshow("output image greyPic", greyPic);
	 	
	
	//5.转为二值图片
	Mat binPic;
	threshold(greyPic, binPic, 88, 255, THRESH_BINARY);    //阈值化为二值图片
	namedWindow("output image binPic",0); 
	imshow("output image binPic", binPic);
	
	//Mat dilated_edges;
	//dilate(binPic, dilated_edges, Mat(), cv::Point(-1, -1), 3, 1, 1); 
	//namedWindow("dilated_edges image",0); 
	//imshow("dilated_edges image", dilated_edges);


	//vector<Point> biggestContour =  FindBigestContour(binPic);//寻找最大轮廓
	//Rect boundRect = boundingRect( Mat(biggestContour) ); //获得轮廓最小外接矩形
	//rectangle(srcPic,boundRect,Scalar(0,0,255));

	//namedWindow("output image srcPic"); 
	//imshow("output image srcPic", srcPic);

#if 1
	//6.进行Canny边缘检测
	Mat cannyPic;
	double cannyThr = 200, FACTOR = 2.5;
	Canny(binPic, cannyPic, cannyThr, cannyThr*FACTOR);    //Canny边缘检测
	
	//7.提取轮廓
	vector<vector<Point> > contours;    //储存轮廓
	vector<Vec4i> hierarchy;
    
	findContours(cannyPic, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE);    //获取轮廓
	namedWindow("output image cannyPic",0); 
	imshow("output image cannyPic", cannyPic);
	Mat linePic;
	linePic = Mat::zeros(cannyPic.rows, cannyPic.cols, CV_8UC3);
	for (int index = 0; index < contours.size(); index++){        
    	drawContours(linePic, contours, index, Scalar(rand() & 255, rand() & 255, rand() & 255), 1, 8/*, hierarchy*/);
	}
	namedWindow("output image linePic",0); 
	imshow("output image linePic", linePic);	
	/*int icount = contours.size();
	printf("icount:%d\n",icount);

	int mycount = 0;
	for(int i=0;i<contours.size();i++)
		for(int j=0;j<contours[i].size();j++){
			printf("(%d,%d)\n",contours[i][j].x,contours[i][j].y);
			mycount++;
	}
	printf("mycount:%d\n",mycount);*/

#if 0
	//遍历轮廓，求出所有支撑角度
    
    float fmax = -1;//用于保存局部最大值
    int   imax = -1;
    bool  bstart = false;
    for (int i=0;i<contours.size();i++){
        Point2f pa = (Point2f)contours[(i+icount-7)%icount];
        Point2f pb = (Point2f)contours[(i+icount+7)%icount];
        Point2f pc = (Point2f)contours[i];
        //两支撑点距离
        float fa = getDistance(pa,pb);
        float fb = getDistance(pa,pc)+getDistance(pb,pc);
        float fang = fa/fb;
        float fsharp = 1-fang;
        if (fsharp>0.05){
            bstart = true;
            if (fsharp>fmax){
                fmax = fsharp;
                imax = i;
            }
        }else{
            if (bstart){
                circle(cannyPic,contours[imax],10,Scalar(255),1);
                circle(srcPic,contours[imax],10,Scalar(255,255,255),1);
                imax  = -1;
                fmax  = -1;
                bstart = false;
            }
        }
    }
#endif
	
	//8.提取面积最大的轮廓并用多边形将轮廓包围
	vector<vector<Point> > polyContours(contours.size());
	int maxArea = 0;
	for (int index = 0; index < contours.size(); index++){        
        if (contourArea(contours[index]) > contourArea(contours[maxArea]))
        	maxArea = index;        
        approxPolyDP(contours[index], polyContours[index], 10, true);
    }
	printf("maxArea:%d---maxArea:%lf\n\n",maxArea,contourArea(contours[maxArea]));
	Mat polyPic = Mat::zeros(srcPic.size(), CV_8UC3);
	drawContours(polyPic, polyContours, maxArea, Scalar(0,0,255/*rand() & 255, rand() & 255, rand() & 255*/), 2);
	
	namedWindow("output image polyPic",0); 
	imshow("output image polyPic", polyPic);

	
	//9.寻找凸包
	vector<int>  hull;
	convexHull(polyContours[maxArea], hull, false);    //检测该轮廓的凸包 

	for (int i = 0; i < hull.size(); ++i){
        circle(polyPic, polyContours[maxArea][i], 10, Scalar(rand() & 255, rand() & 255, rand() & 255), 3);
   	 }
	addWeighted(polyPic, 1, srcPic, 1, 0, srcPic);

	namedWindow("output image srcPic2",0); 
	imshow("output image srcPic2", polyPic);
	for (int i = 0; i < 4; i++){
    	polyContours[maxArea][i] = Point2f(polyContours[maxArea][i].x, polyContours[maxArea][i].y); //恢复坐标到原图
		//cout<<"polyContours["<<maxArea<<"]"<<"["<<i<<"]: "<<polyContours[maxArea][i]<<endl;
	}
	for (int i = 0; i < 4; i++){
		cout<<"polyContours["<<maxArea<<"]"<<"["<<i<<"]: "<<polyContours[maxArea][i]<<endl;
	}

#if 1
	//10.投影变换
	Point2f srcPoints_man[4], srcPoints[4],dstPoints[4];
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(srcPic.cols, 0);
	dstPoints[2] = Point2f(srcPic.cols, srcPic.rows);
	dstPoints[3] = Point2f(0, srcPic.rows);
	
	cout << "dstPoints[0]: " << dstPoints[0] << endl;
	cout << "dstPoints[1]: " << dstPoints[1] << endl;
	cout << "dstPoints[2]: " << dstPoints[2] << endl;
	cout << "dstPoints[3]: " << dstPoints[3] << endl<<endl;
	
		
	
	
	
    //对四个点进行排序 分出左上 右上 右下 左下
	bool sorted = false;
	int n = 4;
	while (!sorted){
    	for (int i = 1; i < n; i++){
    	sorted = true;
        	if (polyContours[maxArea][i-1].x > polyContours[maxArea][i].x){
            	swap(polyContours[maxArea][i-1], polyContours[maxArea][i]);
            	sorted = false;
        	}
    	}
    	n--;
	}
	
	cout<<endl;
	if (polyContours[maxArea][0].y < polyContours[maxArea][1].y){
    	srcPoints[0] = polyContours[maxArea][0];
    	srcPoints[3] = polyContours[maxArea][1];
	}
	else{
    	srcPoints[0] = polyContours[maxArea][1];
    	srcPoints[3] = polyContours[maxArea][0];
	}

	if (polyContours[maxArea][2].y < polyContours[maxArea][3].y){
    	srcPoints[1] = polyContours[maxArea][2];
    	srcPoints[2] = polyContours[maxArea][3];
	}
	else{
    	srcPoints[1] = polyContours[maxArea][3];
    	srcPoints[2] = polyContours[maxArea][2];
	}

	
	cout << "srcPoints[0]: "<< srcPoints[0] << endl;
	cout << "srcPoints[1]: "<< srcPoints[1] << endl;
	cout << "srcPoints[2]: "<< srcPoints[2] << endl;
	cout << "srcPoints[3]: "<< srcPoints[3] << endl<<endl;
	
	/*
	srcPoints_man[0] = Point2f(56, 38);
	srcPoints_man[1] = Point2f(566, 228);
	srcPoints_man[2] = Point2f(600, 508);
	srcPoints_man[3] = Point2f(223, 524);
	
	cout << "srcPoints_man[0]: "<< srcPoints_man[0] << endl;
	cout << "srcPoints_man[1]: "<< srcPoints_man[1] << endl;
	cout << "srcPoints_man[2]: "<< srcPoints_man[2] << endl;
	cout << "srcPoints_man[3]: "<< srcPoints_man[3] << endl;
	*/
	
	
	
	
	//Mat transMat = getPerspectiveTransform(srcPoints_man, dstPoints);    //得到变换矩阵
	Mat transMat = getPerspectiveTransform(srcPoints, dstPoints);    //得到变换矩阵	
	//namedWindow("output image transMat",0); 
	//imshow("output image transMat", transMat);	
	
	Mat outPic;
	warpPerspective(srcPic, outPic, transMat, srcPic.size());    //进行坐标变换
	namedWindow("output image outPic",0); 
	imshow("output image outPic", outPic);
#endif
#endif
	waitKey(0);    
	return 0; 
}

