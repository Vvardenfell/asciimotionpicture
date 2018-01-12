// OpenCVLiveLoopVC12.cpp : Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include "MonoLoop.h"
#include "StereoLoop.h"
#include "output.h"
#include <iostream>
#include <memory>
using namespace std;

const int MY_IMAGE_WIDTH  = 640;
const int MY_IMAGE_HEIGHT = 480;
const int MY_WAIT_IN_MS   = 20;

const std::size_t BUFFER_WIDTH = 80, BUFFER_HEIGHT = 24;


const ushort CANNY_THRESHOLD = 16000;


int MonoLoopOldStyle()
{
  IplImage* grabImage = 0;
  IplImage* resultImage = 0;
  int key;

  // create window for live video
  cvNamedWindow("Live", CV_WINDOW_AUTOSIZE);
  // create connection to camera
  CvCapture* capture = cvCaptureFromCAM(0);
  // init camera
  cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH, MY_IMAGE_WIDTH );
  cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT, MY_IMAGE_HEIGHT );

  // check connection to camera by grabbing a frame
  if(!cvGrabFrame(capture))
  {
    cvReleaseCapture(&capture);
    cvDestroyWindow("Live");
    printf("Could not grab a frame\n\7");
    return -1;
  }

  // retrieve the captured frame
  grabImage=cvRetrieveFrame(capture);
  // init result image, e.g. with size and depth of grabImage
  resultImage = cvCreateImage(cvGetSize(grabImage), grabImage->depth, grabImage->nChannels);

  bool continueGrabbing = true;
  while (continueGrabbing)
  {
    if(!cvGrabFrame(capture))
    {
      cvReleaseCapture(&capture);
      cvDestroyWindow("Live");
      cvReleaseImage(&grabImage);
      printf("Could not grab a frame\n\7");
      return -1;
    }
    else
    {
      grabImage = cvRetrieveFrame(capture);

      /*******************************todo*****************************/
        cvCopy(grabImage, resultImage, NULL );
      /***************************end todo*****************************/

      cvShowImage("Live", resultImage);

      key = cvWaitKey(MY_WAIT_IN_MS);

      if (key == 27)
        continueGrabbing = false;
    }
  }

  // release all
  cvReleaseCapture(&capture);
  cvDestroyWindow("Live");
  cvReleaseImage(&resultImage);

  return 0;
}

int MonoLoop(HINSTANCE hInstance, int iCmdShow)
{
  cv::VideoCapture cap(0);

  if(!cap.isOpened())
  {
    cout << "Cannot open the video cam" << endl;
    return -1;
  }

  // Set cameras to 15fps (if wanted!!!)
  cap.set(CV_CAP_PROP_FPS, 15);

  double dWidth = cap.get(CV_CAP_PROP_FRAME_WIDTH);
  double dHeight = cap.get(CV_CAP_PROP_FRAME_HEIGHT);

  // Set image size
  cap.set(CV_CAP_PROP_FRAME_WIDTH, MY_IMAGE_WIDTH);
  cap.set(CV_CAP_PROP_FRAME_HEIGHT, MY_IMAGE_HEIGHT);

  // display the frame size that OpenCV has picked in order to check
  cout << "cam Frame size: " << dWidth << " x " << dHeight << endl;
  cv::namedWindow("cam",CV_WINDOW_AUTOSIZE);


  FrameBuffer frame(BUFFER_WIDTH, BUFFER_HEIGHT);
  Windows::frame = &frame;
  Windows::init(hInstance, iCmdShow, "ASCII Motion Picture");



  cv::Mat inputFrame;
  cv::Mat outputFrame;
  cv::Mat gray_scale_image;
  cv::Mat detected_edges_image;
  cv::Mat gaussian_filtered;
  cv::Mat grad_x, grad_y;
  cv::Mat direction, strength;
  cv::Mat mask, grays;
  int scale = 1;
  int delta = 0;
  int depth = CV_16S;

  cv::Mat angleLUT(1, 256, CV_8U);
  uchar *p = angleLUT.data;
  for (int i = 0; i < 256; i++)
  {
	  if (i < 16) p[i] = 180;
	  else if (i < 48) p[i] = 45;
	  else if (i < 79) p[i] = 90;
	  else if (i < 111) p[i] = 135;
	  else if (i <143) p[i] = 180;
	  else if (i < 174) p[i] = 45;
	  else if (i < 206) p[i] = 90;
	  else if (i < 238) p[i] = 135;
	  else if (i < 254) p[i] = 180;
	  else p[i] = 0;
  }

  while(1)
  {

    bool bSuccess = cap.read(inputFrame);

    if (!bSuccess)
    {
      cout << "Cannot read a frame from video stream" << endl;
      break;
    }

    /*******************************todo*****************************/

	// canny preparation
	outputFrame.create(inputFrame.size(), inputFrame.type());
	cv::cvtColor(inputFrame, gray_scale_image, CV_BGR2GRAY);
	//cv::blur(gray_scale_image, detected_edges_image, cv::Size(3,3));
	cv::GaussianBlur(gray_scale_image, gaussian_filtered, cv::Size(5, 5), 0, 0);
	cv::Sobel(gaussian_filtered, grad_x, depth, 1, 0, 7, scale, delta, 4);
	cv::Sobel(gaussian_filtered, grad_y, depth, 0, 1, 7, scale, delta, 4);

	// Canny
	direction = cv::Mat(grad_x.rows, grad_x.cols, CV_8U);
	strength = cv::Mat(grad_x.rows, grad_x.cols, CV_16U);

	int nRows = grad_x.rows;
	int nCols = grad_x.cols;
	// if image is stored in a continuous segment we can loose one loop  => speed up
	if (grad_x.isContinuous())
	{
		nCols *= nRows;
		nRows = 1;
	}
	//calculate direction and strength of the edges
	{
		short *x, *y;
		ushort *erg;
		uchar *dir;
		for (int i = 0; i < nRows; ++i)
		{
			x = grad_x.ptr<short>(i);
			y = grad_y.ptr<short>(i);
			erg = strength.ptr<ushort>(i);					// improve by getting rid of erg
			dir = direction.ptr<uchar>(i);
			for (int j = 0; j < nCols; ++j)
			{
				erg[j] = cv::sqrt(pow(x[j], 2) + pow(y[j], 2));
				if (erg[j] > CANNY_THRESHOLD)
				{
					dir[j] = cv::fastAtan2(y[j], x[j]/1.42);
				}
				else dir[j] = 255;
			}
		}
	}
	// filter directions
	cv::LUT(direction, angleLUT, direction); //an continuous approach might give better results in rare cases
	// generate mask
	mask = cv::Mat(direction.rows/8, (direction.cols/8)*5, CV_8U, cv::Scalar(0));
	//cv::resize(direction, mask, cv::Size(80, 60), 0, 0, CV_INTER_AREA);
	//cv::resize(gaussian_filtered, grays, cv::Size(80,60), 0, 0, CV_INTER_AREA); 
	//cv::LUT(mask, angleLUT, mask);
	nRows = direction.rows;
	nCols = direction.cols;
	{
		uchar *dir, *erg;
		int maskRow, maskCol;
		for (int i = 0; i < nRows; i++)
		{
			erg = mask.ptr<uchar>(i / 8);
			dir = direction.ptr<uchar>(i);
			for (int j = 0; j < nCols; j++)
			{
				maskCol = j / 8;
				switch (dir[j])
				{
				case 45:
					erg[maskCol]++;
				case 90:
					erg[maskCol+1]++;
				case 135:
					erg[maskCol+2]++;
				case 180:
					erg[maskCol+3]++;
				default:
					erg[maskCol+4]++;
				}

				if (dir[j] == 0) frame.render('0', j, i);
				else if (dir[j] == 45) frame.render('\\', j, i);
				else if (dir[j] == 135) frame.render('/', j, i);
				else if (dir[j] == 90) frame.render('|', j, i);
				else if (dir[j] == 180) frame.render('-', j, i);
				else frame.render('E', j, i);
			}
		}

		Windows::redraw();
	}

	outputFrame = direction;
    /***************************end todo*****************************/

    imshow("cam", outputFrame);
    if(cv::waitKey(MY_WAIT_IN_MS) == 27)
    {
      cout << "ESC key is pressed by user" << endl;
      break;
    }
	//char input;
	//cin >> input;
  }
  return 0;
}

int StereoLoop()
{
  cv::VideoCapture cap1(0);
  cv::VideoCapture cap2(1);

  if(!cap1.isOpened())
  {
    cout << "Cannot open the video cam [0]" << endl;
    return -1;
  }

  if(!cap2.isOpened())
  {
    cout << "Cannot open the video cam [1]" << endl;
    return -1;
  }

  // Set cameras to 15fps (if wanted!!!)
  cap1.set(CV_CAP_PROP_FPS, 15);
  cap2.set(CV_CAP_PROP_FPS, 15);

  double dWidth1 = cap1.get(CV_CAP_PROP_FRAME_WIDTH);
  double dHeight1 = cap1.get(CV_CAP_PROP_FRAME_HEIGHT);
  double dWidth2 = cap2.get(CV_CAP_PROP_FRAME_WIDTH);
  double dHeight2 = cap2.get(CV_CAP_PROP_FRAME_HEIGHT);

  // Set image size
  cap1.set(CV_CAP_PROP_FRAME_WIDTH, MY_IMAGE_WIDTH);
  cap1.set(CV_CAP_PROP_FRAME_HEIGHT, MY_IMAGE_HEIGHT);
  cap2.set(CV_CAP_PROP_FRAME_WIDTH, MY_IMAGE_WIDTH);
  cap2.set(CV_CAP_PROP_FRAME_HEIGHT, MY_IMAGE_HEIGHT);

  // display the frame size that OpenCV has picked in order to check
  cout << "cam[0] Frame size: " << dWidth1 << " x " << dHeight1 << endl;
  cout << "cam[1] Frame size: " << dWidth2 << " x " << dHeight2 << endl;
  cv::namedWindow("cam[0]",CV_WINDOW_AUTOSIZE);
  cv::namedWindow("cam[1]",CV_WINDOW_AUTOSIZE);

  cv::Mat inputFrame1, inputFrame2;
  cv::Mat outputFrame1, outputFrame2;

  while(1)
  {

    bool bSuccess1 = cap1.read(inputFrame1);
    bool bSuccess2 = cap2.read(inputFrame2);

    if (!bSuccess1)
    {
      cout << "Cannot read a frame from video stream [0]" << endl;
      break;
    }

    if (!bSuccess2)
    {
      cout << "Cannot read a frame from video stream [1]" << endl;
      break;
    }


    /*******************************todo*****************************/
    outputFrame1 = inputFrame1;
    outputFrame2 = inputFrame2;
    /***************************end todo*****************************/


    imshow("cam[0]", outputFrame1);
    imshow("cam[1]", outputFrame2);

    if(cv::waitKey(MY_WAIT_IN_MS) == 27)
    {
      cout << "ESC key is pressed by user" << endl;
      break;
    }
  }
  return 0;
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {

    CMonoLoop myLoop;
    //  CStereoLoop myLoop;
    // myLoop.Run();

    return MonoLoop(hInstance, iCmdShow);
}

/*
int _tmain(int argc, _TCHAR* argv[])
{
  CMonoLoop myLoop;
//  CStereoLoop myLoop;
 // myLoop.Run();

  return MonoLoop();
}*/

