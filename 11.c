#include <stdio.h>
#include <time.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include "deblur.h"
#define PATCH_SIZE 11
#define DEBLUR_WIN_SIZE 11
#define SIGMA_W 10
#define LAMBDA 10
#define LEFT_TOP_CORNER 1
#define TOP 2
#define RIGHT_TOP_CORNER 3
#define LEFT 4
#define CENTER 5 
#define RIGHT 6
#define LEFT_BOTTOM_CORNER 7
#define BOTTOM 8
#define RIGHT_BOTTOM_CORNER 9
#define P(E) printf(#E"=%d\n",E);
#define PF(E) printf(#E"=%f\n",E);
extern IplImage *images[MAX_IMAGE];
extern IplImage *images_luck[MAX_IMAGE];
extern CvMat *hom[MAX_IMAGE][MAX_IMAGE];
extern CvSize image_size;
// 用OpenCv函数实现，但是性能不佳
// 纯手工，性能比上面那个好
double sqrdiff(const IplImage *p1, const IplImage *p2)
{
	CvRect roi = cvGetImageROI(p1);
	int v = p1->nChannels;
	double sum = 0;
	for (int i = 0; i < roi.height; ++i)
		for (int j = 0; j < roi.width; ++j)
		{
			CvScalar s1 = cvGet2D(p1, i, j);
			CvScalar s2 = cvGet2D(p2, i, j);
			for (int k = 0; k < v; ++k)
			{
				double t = s1.val[k]-s2.val[k];
				sum += t*t;
			}
		}
	return sum;
}
static double luck_diff(const IplImage *luck)
{
	CvRect roi = cvGetImageROI(luck);
	double sum = 0;
	for (int i = 0; i < roi.height; ++i)
		for (int j = 0; j < roi.width; ++j)
		{
			CvScalar s = cvGet2D(luck, i, j);
			double t = 1-s.val[3];
			sum += t*t;
		}
	return sum;
}
static void search_aux(IplImage *blur , IplImage *image , IplImage *luck ,int points[][2] , int pointsNum ,  int* index , double* centerDiff)
{
	double mindiff = *centerDiff;
	double diff , diff2 ; 
	int pos = 1 ;
	for( int i = 0 ; i < pointsNum ; i++ )
	{
		cvSetImageROI(blur, cvRect(points[i][0]-PATCH_SIZE/2, points[i][1]-PATCH_SIZE/2, PATCH_SIZE, PATCH_SIZE));
		diff = sqrdiff(image , blur) ;
		cvSetImageROI(luck, cvRect(points[i][0]-PATCH_SIZE/2, points[i][1]-PATCH_SIZE/2, PATCH_SIZE, PATCH_SIZE));
		diff2 = luck_diff(luck);
		diff += LAMBDA * diff2 ; 
		if( diff < mindiff )
		{
			*index = pos ;
			mindiff = diff;
		}
		++pos ;
	}
	*centerDiff = mindiff ;
}
static void  searchLeftTopCorner(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff)
{
	int points[5][2] = {{*x - 2 , *y + 2} , {*x - 2 , *y } ,{*x - 2 , *y - 2 } ,{*x , *y - 2 }, {*x + 2 , *y - 2}} ;
	int status[6] = { CENTER ,LEFT_BOTTOM_CORNER , LEFT , LEFT_TOP_CORNER , TOP , RIGHT_TOP_CORNER };
	int pos = 0 ;
    search_aux(blur , image , luck , points , 5 , &pos , centerDiff);
	*index = status[pos];	
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void  searchRightTopCorner(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff)
{
	int points[5][2] = {{*x - 2 , *y - 2} , {*x  , *y - 2 } ,{*x + 2 , *y - 2 } ,{*x + 2 , *y }, {*x + 2 , *y + 2}} ;
	int status[6] = { CENTER , LEFT_TOP_CORNER , TOP , RIGHT_TOP_CORNER , RIGHT , RIGHT_BOTTOM_CORNER};
	int pos = 0 ;
    search_aux(blur , image , luck , points , 5 , &pos , centerDiff);
	*index = status[pos];	
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void  searchLeftBottomCorner(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff)
{
	int points[5][2] = {{*x - 2 , *y - 2} , {*x - 2  , *y } ,{*x - 2 , *y + 2 } ,{*x , *y + 2 }, {*x + 2 , *y + 2}} ;
	int status[6] = { CENTER , LEFT_TOP_CORNER , LEFT  , LEFT_BOTTOM_CORNER, BOTTOM , RIGHT_BOTTOM_CORNER};
	int pos = 0 ;
    search_aux(blur , image , luck , points , 5 , &pos , centerDiff);
	*index = status[pos];	
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void  searchRightBottomCorner(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff)
{
	int points[5][2] = {{*x - 2 , *y + 2} , {*x  , *y + 2 } ,{*x + 2 , *y + 2 } ,{*x + 2 , *y }, {*x + 2 , *y - 2}} ;
	int status[6] = { CENTER , LEFT_BOTTOM_CORNER, BOTTOM , RIGHT_BOTTOM_CORNER, RIGHT, RIGHT_TOP_CORNER};
	int pos = 0 ;
    search_aux(blur , image , luck , points , 5 , &pos , centerDiff);
	*index = status[pos];	
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void searchTop(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff) 
{
	int points[3][2] = {{*x - 2  , *y - 2} ,{ *x , *y -2 } , {*x + 2 , *y - 2 }} ;
	int status[4] = { CENTER , LEFT_TOP_CORNER , TOP , RIGHT_TOP_CORNER} ;
	int pos = 0 ;
	search_aux(blur , image , luck , points , 3 , &pos , centerDiff);
	*index = status[pos] ;
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void searchRight(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff) 
{
	int points[3][2] = {{*x + 2  , *y - 2} ,{ *x + 2, *y } , {*x + 2 , *y + 2 }} ;
	int status[4] = { CENTER , RIGHT_TOP_CORNER , RIGHT , RIGHT_BOTTOM_CORNER} ;
	int pos = 0 ;
	search_aux(blur , image , luck , points , 3 , &pos , centerDiff);
	*index = status[pos] ;
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void searchBottom(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff) 
{
	int points[3][2] = {{*x - 2  , *y + 2} ,{ *x , *y + 2 } , {*x + 2 , *y + 2 }} ;
	int status[4] = { CENTER , LEFT_BOTTOM_CORNER , BOTTOM , RIGHT_BOTTOM_CORNER} ;
	int pos = 0 ;
	search_aux(blur , image , luck , points , 3 , &pos , centerDiff);
	*index = status[pos] ;
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static void searchLeft(IplImage *blur , IplImage *image , IplImage *luck , int *x , int *y , int* index , double* centerDiff) 
{
	int points[3][2] = {{*x - 2  , *y - 2} ,{ *x - 2, *y } , {*x - 2 , *y + 2 }} ;
	int status[4] = { CENTER , LEFT_TOP_CORNER , LEFT  , LEFT_BOTTOM_CORNER} ;
	int pos = 0 ;
	search_aux(blur , image , luck , points , 3 , &pos , centerDiff);
	*index = status[pos] ;
	if( CENTER != *index )
	{
		*x = points[pos-1][0] ;
		*y = points[pos-1][1] ;
	}
}
static double search(IplImage *blur , IplImage *image , IplImage *luck ,  int* x , int* y , int* index , int step)
{
	int minx = -1;
	int miny = -1;
	double mindiff = 20000000000 ;
	double diff , diff2 ; 
	int pos = 1 ;
	for( int i = *x - step ; i <= *x + step ; i += step)
	{
		for( int j = *y - step; j <= *y + step ; j += step)
		{
			cvSetImageROI(blur, cvRect(i-PATCH_SIZE/2, j-PATCH_SIZE/2, PATCH_SIZE, PATCH_SIZE));
			diff = sqrdiff(image , blur) ;
			cvSetImageROI(luck, cvRect(i-PATCH_SIZE/2, j-PATCH_SIZE/2, PATCH_SIZE, PATCH_SIZE));
			diff2 = luck_diff(luck);
			diff += LAMBDA * diff2 ; 
			if( diff < mindiff )
			{
				*index = pos ;
				mindiff = diff;
				minx = i;
				miny = j;
			}
			++pos ;
		}
	}
	*x = minx ;
	*y = miny ;
	return mindiff ; 
}
static int deblur_patch(IplImage *blur[], IplImage *luck[], int image_num, int n, int x, int y, CvScalar *res)
{
	if (n < 1 || n >= image_num-1 || x-PATCH_SIZE/2 < 0 || y-PATCH_SIZE/2 < 0 || x+PATCH_SIZE/2 >= image_size.width || y+PATCH_SIZE/2 >= image_size.height) return -1;
	cvSetImageROI(images[n], cvRect(x-PATCH_SIZE/2, y-PATCH_SIZE/2, PATCH_SIZE, PATCH_SIZE));
	int left = x-DEBLUR_WIN_SIZE/2-PATCH_SIZE/2;
	left = left < 0 ? PATCH_SIZE/2 : left+PATCH_SIZE/2;
	int right = x+DEBLUR_WIN_SIZE/2+PATCH_SIZE/2;
	right = right >= image_size.width ? image_size.width-1-PATCH_SIZE/2 : right-PATCH_SIZE/2;
	int top = y-DEBLUR_WIN_SIZE/2-PATCH_SIZE/2;
	top = top < 0 ? PATCH_SIZE/2 : top+PATCH_SIZE/2;
	int bottom = y+DEBLUR_WIN_SIZE/2+PATCH_SIZE/2;
	bottom = bottom >= image_size.height ? image_size.height-1-PATCH_SIZE/2 : bottom-PATCH_SIZE/2;
	int minj = -21;
	int minx = -21;
	int miny = -21;
	int mx = x , my = y ; 
	double mindiff = 200000000000 ;
	double tmpDiff = 200000000000 ;
	int pos = 0 ;
	for (int j = 0; j < image_num; ++j)
	{
		pos = 0 ;
		if( mx + 2 <= right && mx - 2 >= left
				&& my + 2 <= bottom && my -2 >= top)
		{
			int searchX , searchY ;
			searchX = mx ;
            searchY = my ;
			tmpDiff = search(blur[j] , images[n] , luck[j] , &searchX , &searchY , &pos , 2) ;
			if( pos == CENTER )
			{
				mindiff = search(blur[j] , images[n] , luck[j] , &searchX , &searchY , &pos , 1 ) ;
				minj = j ;
				minx = searchX ;
				miny = searchY ;
				continue ;
			} else 
			{
				typedef void (*searchFuncPtr)(IplImage*, IplImage*, IplImage*, int* , int* , int* , double* ) ;
				searchFuncPtr searchFuncArray[10] = {NULL , searchLeftTopCorner , searchTop , searchRightTopCorner  ,
									searchLeft , NULL , searchRight , 
									searchLeftBottomCorner , searchBottom , searchRightBottomCorner} ;
				for( ; CENTER != pos && searchX - 2 >= left && searchX + 2 <= right 
						&& searchY + 2 <= bottom && searchY - 2 >= top ;  )
				{
					searchFuncPtr searchFunc = searchFuncArray[pos] ; 
					searchFunc( blur[j] , images[n] , luck[j] , &searchX , &searchY , &pos , &tmpDiff);
				}
				if( CENTER == pos )
				{
					if( searchX - 1 >= left && searchX + 1 <= right
							&& searchY -1 >= top && searchY + 1 <= bottom)
					{
						mindiff = tmpDiff ;
						minj = j ;
						minx = searchX ;
						miny = searchY ;
						continue ;
					}
					mindiff = search(blur[j] , images[n] , luck[j] , &searchX , &searchY , &pos , 1 ) ;
					minj = j ;
					minx = searchX ;
					miny = searchY ;
					continue ;
				}else
				{
					mindiff = tmpDiff ;
					minj = j ;
					minx = searchX ;
					miny = searchY ;
					continue ;
				}
			}
		}
		cvResetImageROI(blur[j]);
		cvResetImageROI(luck[j]);
	}
	cvResetImageROI(images[n]);
	res->val[0] = minj;
	res->val[1] = minx;
	res->val[2] = miny;
	res->val[3] = mindiff;
	if( minj == -21 )
	{
		return 1 ;	
	}
	return 0;
}
static CvScalar weighted_average(const CvScalar *s, const double *w, int n)
{
	CvScalar res = {{0, 0, 0, 0}};
	if (n > 0)
	{
		double wsum = 0;
		for (int i = 0; i < n; ++i)
		{
			wsum += w[i];
			for (int j = 0; j < 4; ++j)
			{
				res.val[j] += s[i].val[j]*w[i];
			}
		}
		for (int i = 0; i < 4; ++i)
		{
			res.val[i] /= wsum;
		}
	}
	return res;
}
void deblur_image(int image_num, int n, IplImage *result, IplImage *result_luck)
{
	cvSetZero(result);
	cvSetZero(result_luck);
	IplImage *trans[MAX_IMAGE];
	IplImage *trans_luck[MAX_IMAGE];
	IplImage *blur[MAX_IMAGE];
	for (int i = 0; i < image_num; ++i)
	{
		trans[i] = cvCreateImage(image_size, IPL_DEPTH_8U, 3);
		cvWarpPerspective(images[i], trans[i], hom[i][n], CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		trans_luck[i] = cvCreateImage(image_size, IPL_DEPTH_32F, 4);
		cvWarpPerspective(images_luck[i], trans_luck[i], hom[i][n], CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		blur[i] = cvCreateImage(image_size, IPL_DEPTH_8U, 3);
		blur_function(trans[i], blur[i], hom[n-1][n], hom[n][n+1]);
	}
	for (int i = 0; i < image_num; ++i)
	{
		char wname[16];
		sprintf(wname, "Homography%d", i);
		cvNamedWindow(wname, CV_WINDOW_AUTOSIZE);
		cvMoveWindow(wname, i*50, i*50);
		cvShowImage(wname, trans[i]);
		sprintf(wname, "Blurred%d", i);
		cvNamedWindow(wname, CV_WINDOW_AUTOSIZE);
		cvMoveWindow(wname, i*50+100, i*50);
		cvShowImage(wname, blur[i]);
	}
	cvWaitKey(0);
	cvDestroyAllWindows();
	int grid_r = (image_size.height-PATCH_SIZE/2-5) / (PATCH_SIZE/2);
	int grid_c = (image_size.width-PATCH_SIZE/2-5) / (PATCH_SIZE/2);
	if (grid_r > 0 && grid_c > 0)
	{
		CvMat *patch = cvCreateMat(grid_r, grid_c, CV_64FC4);
		for (int i = 0; i < grid_r; ++i)
		{
			int y = (i+1)*(PATCH_SIZE/2)+2;
			int t1 = clock();
			for (int j = 0; j < grid_c; ++j)
			{
				CvScalar res;
				int x = (j+1)*(PATCH_SIZE/2)+2;
				if (deblur_patch(blur, trans_luck, image_num, n, x, y, &res) != 0)
				{
					printf("deblur_patch: %d:%d,%d failed.\n", n, x, y);
					res.val[0] = n;
					res.val[1] = x;
					res.val[2] = y;
					res.val[3] = 0;
				}
				res.val[3] = exp(-res.val[3]/(2*SIGMA_W*SIGMA_W));
				CV_MAT_ELEM(*patch, CvScalar, i, j) = res;
			}
			int t2 = clock();
			printf("y:%d/%d  %d ms\n", y, image_size.height, (t2-t1)*1000/CLOCKS_PER_SEC);
		}
		cvNamedWindow("origin", CV_WINDOW_AUTOSIZE);
		cvShowImage("origin", images[n]);
		cvNamedWindow("result", CV_WINDOW_AUTOSIZE);
		printf("deblur starting ...\n") ;
		// 中心部分
		for (int i = 1; i < grid_r; ++i)
		{
			int miny = i*(PATCH_SIZE/2);
			for (int j = 1; j < grid_c; ++j)
			{
				CvScalar pres1 = CV_MAT_ELEM(*patch, CvScalar, i-1, j-1);
				CvScalar pres2 = CV_MAT_ELEM(*patch, CvScalar, i-1, j);
				CvScalar pres3 = CV_MAT_ELEM(*patch, CvScalar, i, j-1);
				CvScalar pres4 = CV_MAT_ELEM(*patch, CvScalar, i, j);
				int minx = j*(PATCH_SIZE/2);
				for (int y = 0; y < PATCH_SIZE/2; ++y)
					for (int x = 0; x < PATCH_SIZE/2; ++x)
					{
						CvScalar v[4];
						v[0] = cvGet2D(trans[(int)pres1.val[0]], (int)pres1.val[2]+y, (int)pres1.val[1]+x);
						v[1] = cvGet2D(trans[(int)pres2.val[0]], (int)pres2.val[2]+y, (int)pres2.val[1]+x-PATCH_SIZE/2);
						v[2] = cvGet2D(trans[(int)pres3.val[0]], (int)pres3.val[2]+y-PATCH_SIZE/2, (int)pres3.val[1]+x);
						v[3] = cvGet2D(trans[(int)pres4.val[0]], (int)pres4.val[2]+y-PATCH_SIZE/2, (int)pres4.val[1]+x-PATCH_SIZE/2);
						double w[4] = {pres1.val[3], pres2.val[3], pres3.val[3], pres4.val[3]};
						CvScalar pv = weighted_average(v, w, 4);
						cvSet2D(result, y+miny, x+minx, pv);
					}
			}
			cvShowImage("result", result);
			cvWaitKey(20);
		}
		// 四周特殊情况
		for (int y = 0; y < PATCH_SIZE/2; ++y)
			for (int x = 0; x < PATCH_SIZE/2; ++x)
			{
				CvScalar pres = CV_MAT_ELEM(*patch, CvScalar, 0, 0);
				CvScalar pv = cvGet2D(trans[(int)pres.val[0]], (int)pres.val[2]+y-PATCH_SIZE/2, (int)pres.val[1]+x-PATCH_SIZE/2);
				cvSet2D(result, y, x, pv);
				pres = CV_MAT_ELEM(*patch, CvScalar, 0, grid_c-1);
				pv = cvGet2D(trans[(int)pres.val[0]], (int)pres.val[2]+y-PATCH_SIZE/2, (int)pres.val[1]+x);
				cvSet2D(result, y, grid_c*(PATCH_SIZE/2)+x, pv);
				pres = CV_MAT_ELEM(*patch, CvScalar, grid_r-1, 0);
				pv = cvGet2D(trans[(int)pres.val[0]], (int)pres.val[2]+y, (int)pres.val[1]+x-PATCH_SIZE/2);
				cvSet2D(result, grid_r*(PATCH_SIZE/2)+y, x, pv);
				pres = CV_MAT_ELEM(*patch, CvScalar, grid_r-1, grid_c-1);
				pv = cvGet2D(trans[(int)pres.val[0]], (int)pres.val[2]+y, (int)pres.val[1]+x);
				cvSet2D(result, grid_r*(PATCH_SIZE/2)+y, grid_c*(PATCH_SIZE/2)+x, pv);
			}
		for (int j = 1; j < grid_c; ++j)
		{
			CvScalar pres1 = CV_MAT_ELEM(*patch, CvScalar, 0, j-1);
			CvScalar pres2 = CV_MAT_ELEM(*patch, CvScalar, 0, j);
			CvScalar pres3 = CV_MAT_ELEM(*patch, CvScalar, grid_r-1, j-1);
			CvScalar pres4 = CV_MAT_ELEM(*patch, CvScalar, grid_r-1, j);
			int minx = j*(PATCH_SIZE/2);
			for (int y = 0; y < PATCH_SIZE/2; ++y)
				for (int x = 0; x < PATCH_SIZE/2; ++x)
				{
					CvScalar v[2];
					v[0] = cvGet2D(trans[(int)pres1.val[0]], (int)pres1.val[2]+y-PATCH_SIZE/2, (int)pres1.val[1]+x);
					v[1] = cvGet2D(trans[(int)pres2.val[0]], (int)pres2.val[2]+y-PATCH_SIZE/2, (int)pres2.val[1]+x-PATCH_SIZE/2);
					double w[2] = {pres1.val[3], pres2.val[3]};
					CvScalar pv = weighted_average(v, w, 2);
					cvSet2D(result, y, minx+x, pv);
					v[0] = cvGet2D(trans[(int)pres3.val[0]], (int)pres3.val[2]+y, (int)pres3.val[1]+x);
					v[1] = cvGet2D(trans[(int)pres4.val[0]], (int)pres4.val[2]+y, (int)pres4.val[1]+x-PATCH_SIZE/2);
					w[0] = pres3.val[3];
					w[0] = pres4.val[3];
					pv = weighted_average(v, w, 2);
					cvSet2D(result, grid_r*(PATCH_SIZE/2)+y, minx+x, pv);
				}
		}
		for (int i = 1; i < grid_r; ++i)
		{
			CvScalar pres1 = CV_MAT_ELEM(*patch, CvScalar, i-1, 0);
			CvScalar pres2 = CV_MAT_ELEM(*patch, CvScalar, i, 0);
			CvScalar pres3 = CV_MAT_ELEM(*patch, CvScalar, i-1, grid_c-1);
			CvScalar pres4 = CV_MAT_ELEM(*patch, CvScalar, i, grid_c-1);
			int miny = i*(PATCH_SIZE/2);
			for (int y = 0; y < PATCH_SIZE/2; ++y)
				for (int x = 0; x < PATCH_SIZE/2; ++x)
				{
					CvScalar v[2];
					v[0] = cvGet2D(trans[(int)pres1.val[0]], (int)pres1.val[2]+y, (int)pres1.val[1]+x-PATCH_SIZE/2);
					v[1] = cvGet2D(trans[(int)pres2.val[0]], (int)pres2.val[2]+y-PATCH_SIZE/2, (int)pres2.val[1]+x-PATCH_SIZE/2);
					double w[2] = {pres1.val[3], pres2.val[3]};
					CvScalar pv = weighted_average(v, w, 2);
					cvSet2D(result, miny+y, x, pv);
					v[0] = cvGet2D(trans[(int)pres3.val[0]], (int)pres3.val[2]+y, (int)pres3.val[1]+x);
					v[1] = cvGet2D(trans[(int)pres4.val[0]], (int)pres4.val[2]+y-PATCH_SIZE/2, (int)pres4.val[1]+x);
					w[0] = pres3.val[3];
					w[0] = pres4.val[3];
					pv = weighted_average(v, w, 2);
					cvSet2D(result, miny+y, grid_c*(PATCH_SIZE/2)+x, pv);
				}
		}
		cvShowImage("result", result);
		char name[16];
		sprintf(name, "result%d.png", n);
		cvSaveImage(name, result, NULL);
		sprintf(name, "origin%d.png", n);
		cvSaveImage(name, images[n], NULL);
		cvReleaseMat(&patch);
	}
	for (int i = 0; i < image_num; ++i)
	{
		cvReleaseImage(&trans[i]);
		cvReleaseImage(&trans_luck[i]);
		cvReleaseImage(&blur[i]);
	}
}
