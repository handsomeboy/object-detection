#include "stdafx.h"
#include "MyForest.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/ml.hpp>
#include <stdlib.h>// srand, rand 
#include <stdio.h>
#include <math.h>
#include <string> 
#include <time.h>       // time 
#include <random>
#include <ctime> 
#include <cstdlib>
#include <tuple>
#include <iostream>




///set opencv and c++ namespaces
using namespace cv::ml;
using namespace cv;
using namespace std;

void visualizeHOG(cv::Mat img, std::vector<float> &feats, cv::HOGDescriptor hog_detector, int scale_factor = 3);
/*
* img - the image used for computing HOG descriptors. **Attention here the size of the image should be the same as the window size of your cv::HOGDescriptor instance **
* feats - the hog descriptors you get after calling cv::HOGDescriptor::compute
* hog_detector - the instance of cv::HOGDescriptor you used
* scale_factor - scale the image *scale_factor* times larger for better visualization
*/
void visualizeHOG(cv::Mat img, std::vector<float> &feats, cv::HOGDescriptor hog_detector, int scale_factor) {

	cv::Mat visual_image;
	cv::resize(img, visual_image, cv::Size(img.cols * scale_factor, img.rows * scale_factor));

	int n_bins = hog_detector.nbins;
	float rad_per_bin = 3.14 / (float)n_bins;
	cv::Size win_size = hog_detector.winSize;
	cv::Size cell_size = hog_detector.cellSize;
	cv::Size block_size = hog_detector.blockSize;
	cv::Size block_stride = hog_detector.blockStride;

	// prepare data structure: 9 orientation / gradient strenghts for each cell
	int cells_in_x_dir = win_size.width / cell_size.width;
	int cells_in_y_dir = win_size.height / cell_size.height;
	int n_cells = cells_in_x_dir * cells_in_y_dir;
	int cells_per_block = (block_size.width / cell_size.width) * (block_size.height / cell_size.height);

	int blocks_in_x_dir = (win_size.width - block_size.width) / block_stride.width + 1;
	int blocks_in_y_dir = (win_size.height - block_size.height) / block_stride.height + 1;
	int n_blocks = blocks_in_x_dir * blocks_in_y_dir;

	float ***gradientStrengths = new float **[cells_in_y_dir];
	int **cellUpdateCounter = new int *[cells_in_y_dir];
	for (int y = 0; y < cells_in_y_dir; y++) {
		gradientStrengths[y] = new float *[cells_in_x_dir];
		cellUpdateCounter[y] = new int[cells_in_x_dir];
		for (int x = 0; x < cells_in_x_dir; x++) {
			gradientStrengths[y][x] = new float[n_bins];
			cellUpdateCounter[y][x] = 0;

			for (int bin = 0; bin < n_bins; bin++)
				gradientStrengths[y][x][bin] = 0.0;
		}
	}


	// compute gradient strengths per cell
	int descriptorDataIdx = 0;


	for (int block_x = 0; block_x < blocks_in_x_dir; block_x++) {
		for (int block_y = 0; block_y < blocks_in_y_dir; block_y++) {
			int cell_start_x = block_x * block_stride.width / cell_size.width;
			int cell_start_y = block_y * block_stride.height / cell_size.height;

			for (int cell_id_x = cell_start_x;
				cell_id_x < cell_start_x + block_size.width / cell_size.width; cell_id_x++)
				for (int cell_id_y = cell_start_y;
					cell_id_y < cell_start_y + block_size.height / cell_size.height; cell_id_y++) {

				for (int bin = 0; bin < n_bins; bin++) {
					float val = feats.at(descriptorDataIdx++);
					gradientStrengths[cell_id_y][cell_id_x][bin] += val;
				}
				cellUpdateCounter[cell_id_y][cell_id_x]++;
			}
		}
	}


	// compute average gradient strengths
	for (int celly = 0; celly < cells_in_y_dir; celly++) {
		for (int cellx = 0; cellx < cells_in_x_dir; cellx++) {

			float NrUpdatesForThisCell = (float)cellUpdateCounter[celly][cellx];

			// compute average gradient strenghts for each gradient bin direction
			for (int bin = 0; bin < n_bins; bin++) {
				gradientStrengths[celly][cellx][bin] /= NrUpdatesForThisCell;
			}
		}
	}


	for (int celly = 0; celly < cells_in_y_dir; celly++) {
		for (int cellx = 0; cellx < cells_in_x_dir; cellx++) {
			int drawX = cellx * cell_size.width;
			int drawY = celly * cell_size.height;

			int mx = drawX + cell_size.width / 2;
			int my = drawY + cell_size.height / 2;

			rectangle(visual_image,
				cv::Point(drawX * scale_factor, drawY * scale_factor),
				cv::Point((drawX + cell_size.width) * scale_factor,
				(drawY + cell_size.height) * scale_factor),
				CV_RGB(100, 100, 100),
				1);

			for (int bin = 0; bin < n_bins; bin++) {
				float currentGradStrength = gradientStrengths[celly][cellx][bin];

				if (currentGradStrength == 0)
					continue;

				float currRad = bin * rad_per_bin + rad_per_bin / 2;

				float dirVecX = cos(currRad);
				float dirVecY = sin(currRad);
				float maxVecLen = cell_size.width / 2;
				float scale = scale_factor / 5.0; // just a visual_imagealization scale,

												  // compute line coordinates
				float x1 = mx - dirVecX * currentGradStrength * maxVecLen * scale;
				float y1 = my - dirVecY * currentGradStrength * maxVecLen * scale;
				float x2 = mx + dirVecX * currentGradStrength * maxVecLen * scale;
				float y2 = my + dirVecY * currentGradStrength * maxVecLen * scale;

				// draw gradient visual_imagealization
				line(visual_image,
					cv::Point(x1 * scale_factor, y1 * scale_factor),
					cv::Point(x2 * scale_factor, y2 * scale_factor),
					CV_RGB(0, 0, 255),
					1);

			}

		}
	}


	for (int y = 0; y < cells_in_y_dir; y++) {
		for (int x = 0; x < cells_in_x_dir; x++) {
			delete[] gradientStrengths[y][x];
		}
		delete[] gradientStrengths[y];
		delete[] cellUpdateCounter[y];
	}
	delete[] gradientStrengths;
	delete[] cellUpdateCounter;
	cv::imshow("HOG vis", visual_image);
	cv::waitKey(-1);
	cv::imwrite("hog_vis.jpg", visual_image);

}

void getInitialDataDescriptors(vector<Mat1f> &features, cv::Mat& labels, int number_of_cat, int nbr_pictures_cat[]);
/* read folder and modify the two entry Mat table to make them contain only the original features and labels
You mignt need to change the string that gives the path
*/

void getInitialDataDescriptors(vector<Mat1f>& features, cv::Mat &labels, int number_of_cat, int nbr_pictures_cat[]) {
	///Variables(careful, not all of them)
	Mat src, src_gray, src_gray_resized;
	Mat test;
	//char* window_name = "HOG Descriptor"; needed to show HOG Descriptor of pictures
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	vector<float> descriptors;

	/// Beginning and end of path to pictures
	string schemetrain = "C:/Users/Kamel GUERDA/Desktop/TDCV_exercice2/data/task2/train/";
	string extension = ".jpg";

	///get in right category/fold
	int image_cat;
	int image_index;
	string s_image_cat;
	string s_image_index;
	string path;

	///Get HOG descriptor of each picture and fill two Mat (one with descriptors, second with labels)
	for (image_cat = 0; image_cat < number_of_cat; image_cat = image_cat + 1) {
		for (image_index = 0; image_index < nbr_pictures_cat[image_cat]; image_index = image_index + 1) {

			///get 
			s_image_cat = to_string(image_cat);
			while (s_image_cat.length() <2) {
				s_image_cat = "0" + s_image_cat;
			}

			///get a specific image
			s_image_index = to_string(image_index);
			while (s_image_index.length() <4) {
				s_image_index = "0" + s_image_index;
			}
			///create complete path for one specific image
			path = schemetrain + s_image_cat + "/" + s_image_index + extension;
			///example path="C:/Users/Kamel GUERDA/Desktop/TDCV_exercice2/data/task2/train/00/0000.jpg";
			src = imread(path);

			///check if picture exist
			if (!src.data)
			{
				exit;
			}

			/// Convert it to gray
			cv::cvtColor(src, src_gray, CV_BGR2GRAY);

			///Scale to 120*120 image
			cv::resize(src_gray, src_gray_resized, Size(120, 120), 0, 0, INTER_AREA);

			HOGDescriptor hog(
				src_gray_resized.size(),//Size(20, 20), //winSize
				Size(20, 20), //blocksize
				Size(10, 10), //blockStride,
				Size(10, 10), //cellSize,
				9, //nbins,
				1, //derivAper,
				-1, //winSigma,
				0, //histogramNormType,
				0.2, //L2HysThresh,
				0,//gammal correction,
				64,//nlevels=64
				1);

			hog.compute(src_gray_resized, descriptors, Size(136, 136), Size(8, 8));
			//visualizeHOG(src_gray_resized, descriptors, hog, 6);
			//waitKey(0);
			Mat1f m1(1, descriptors.size(), descriptors.data());
			features[image_cat].push_back(m1);
			labels.push_back(image_cat);
		}
	}
}

void getExpandedDataDescriptors(vector<Mat1f>  &features, cv::Mat &labels, int number_of_cat, int nbr_pictures_cat[], int rotateOn, int blurOn, int cropOn);
/*	read folder and modify the two entry Mat table to make them cointain the original features + 
if blurOn =1 , features of the blurred images
if rotateOn =1 , features of the blurred images
if slideOn =1 , features of the cutted images
if you don't want one of them, put 0
*/
void getExpandedDataDescriptors(vector<Mat1f> &features, cv::Mat &labels, int number_of_cat, int nbr_pictures_cat[], int rotateOn, int blurOn, int cropOn) {
	///Variables(careful, not all of them)
	Mat src, src_gray, src_gray_resized;
	Mat test;
	//char* window_name = "HOG Descriptor"; needed to show HOG Descriptor of pictures
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	vector<float> descriptors;

	/// Beginning and end of path to pictures
	string schemetrain = "C:/Users/Kamel GUERDA/Desktop/TDCV_exercice2/data/task2/train/";
	string extension = ".jpg";

	///get in right category/fold
	int image_cat;
	int image_index;
	string s_image_cat;
	string s_image_index;
	string path;

	///Random settings
	time_t seconds;
	time(&seconds);
	int srandomNum;
	//cout << "\nseconds" << seconds;
	srand((unsigned int)seconds);

	///Get HOG descriptor of each picture and fill two Mat (one with descriptors, second with labels)
	///For each image class
	for (image_cat = 0; image_cat < number_of_cat; image_cat = image_cat + 1) {
		///For each picture, we compute hog descriptor and add it to Mat feature
		for (image_index = 0; image_index < nbr_pictures_cat[image_cat]; image_index = image_index + 1) {

			///get 
			s_image_cat = to_string(image_cat);
			while (s_image_cat.length() <2) {
				s_image_cat = "0" + s_image_cat;
			}

			///get a specific image
			s_image_index = to_string(image_index);
			while (s_image_index.length() <4) {
				s_image_index = "0" + s_image_index;
			}
			///create complete path for one specific image
			path = schemetrain + s_image_cat + "/" + s_image_index + extension;
			///example path="C:/Users/Kamel GUERDA/Desktop/TDCV_exercice2/data/task2/train/00/0000.jpg";
			src = imread(path);

			///check if picture exist
			if (!src.data)
			{
				exit;
			}

			/// Convert it to gray
			cv::cvtColor(src, src_gray, CV_BGR2GRAY);

			///Scale to 120*120 image
			cv::resize(src_gray, src_gray_resized, Size(120, 120), 0, 0, INTER_AREA);

			HOGDescriptor hog(
				src_gray_resized.size(),//Size(20, 20), //winSize
				Size(20, 20), //blocksize
				Size(10, 10), //blockStride,
				Size(10, 10), //cellSize,
				9, //nbins,
				1, //derivAper,
				-1, //winSigma,
				0, //histogramNormType,
				0.2, //L2HysThresh,
				0,//gammal correction,
				64,//nlevels=64
				1);

			hog.compute(src_gray_resized, descriptors, Size(136, 136), Size(8, 8));
			//visualizeHOG(src_gray_resized, descriptors, hog, 6);
			//waitKey(0);
			Mat1f m1(1, descriptors.size(), descriptors.data());
			features[image_cat].push_back(m1);
			labels.push_back(image_cat);


			///Also, if expansion of data is asked through rotating pictures, we rotate each picture 3 times per 90degres and compute HOG
			if (rotateOn == 1) {
				Mat src_gray_resized_rotated =src_gray_resized;
				for (int cnt_rotation = 0; cnt_rotation < 3;cnt_rotation++) {
					cv::rotate(src_gray_resized_rotated, src_gray_resized_rotated, 0);//last parameter is Rotate_flag, if 0, 90degre clockwise
					HOGDescriptor hog(
						src_gray_resized.size(),//Size(20, 20), //winSize
						Size(20, 20), //blocksize
						Size(10, 10), //blockStride,
						Size(10, 10), //cellSize,
						9, //nbins,
						1, //derivAper,
						-1, //winSigma,
						0, //histogramNormType,
						0.2, //L2HysThresh,
						0,//gammal correction,
						64,//nlevels=64
						1);
					hog.compute(src_gray_resized_rotated, descriptors, Size(136, 136), Size(8, 8));
					//visualizeHOG(src_gray_resized, descriptors, hog, 6);
					//waitKey(0);
					Mat1f m1(1, descriptors.size(), descriptors.data());
					features[image_cat].push_back(m1);
					labels.push_back(image_cat);

					///If blur is on, then we also get blurred image of the rotated images (if rotate was on)
					if (blurOn == 1) {
						for (int sigma = 1; sigma < 6; sigma = sigma + 2) {
							Mat src_gray_resized_rotated_blurred;
							///Apply some blur 
							GaussianBlur(src_gray_resized_rotated, src_gray_resized_rotated_blurred, Size(0, 0), sigma, 0, BORDER_DEFAULT);//when Size(0,0), kernel is computed from sigma, if sigma.y=0,then its equal to sigma.x
							HOGDescriptor hog(
								src_gray_resized.size(),//Size(20, 20), //winSize
								Size(20, 20), //blocksize
								Size(10, 10), //blockStride,
								Size(10, 10), //cellSize,
								9, //nbins,
								1, //derivAper,
								-1, //winSigma,
								0, //histogramNormType,
								0.2, //L2HysThresh,
								0,//gammal correction,
								64,//nlevels=64
								1);

							hog.compute(src_gray_resized_rotated_blurred, descriptors, Size(136, 136), Size(8, 8));
							//visualizeHOG(src_gray_resized, descriptors, hog, 6);
							//waitKey(0);
							Mat1f m1(1, descriptors.size(), descriptors.data());
							features[image_cat].push_back(m1);
							labels.push_back(image_cat);
						}
					}
				}
			}

			///If we want extented datas through blurred images, blur each image with 3 lvl of blurring sigma = 1,3,5
			/// I do not compute rotated pictures of those blurred pictures because they are already in the first if(rotateOn=1)
			if (blurOn == 1) {
				for (int sigma = 1; sigma < 6; sigma = sigma + 2) {
					Mat src_gray_resized_blurred;
					///Apply some blur 
					GaussianBlur(src_gray_resized, src_gray_resized_blurred, Size(0,0) , sigma, 0, BORDER_DEFAULT);//when Size(0,0), kernel is computed from sigma, if sigma.y=0,then its equal to sigma.x
					HOGDescriptor hog(
						src_gray_resized.size(),//Size(20, 20), //winSize
						Size(20, 20), //blocksize
						Size(10, 10), //blockStride,
						Size(10, 10), //cellSize,
						9, //nbins,
						1, //derivAper,
						-1, //winSigma,
						0, //histogramNormType,
						0.2, //L2HysThresh,
						0,//gammal correction,
						64,//nlevels=64
						1);

					hog.compute(src_gray_resized_blurred, descriptors, Size(136, 136), Size(8, 8));
					//visualizeHOG(src_gray_resized, descriptors, hog, 6);
					//waitKey(0);
					Mat1f m1(1, descriptors.size(), descriptors.data());
					features[image_cat].push_back(m1);
					labels.push_back(image_cat);
				}
			}

			/// If cropOn, we augment the data by cropping and resizing the pictures
			/// if rotating and/or blurring is asked, then we will also compute the successive operation on the cropped images
			if (cropOn == 1) {
				for (int nbr_cropped_pictures = 0; nbr_cropped_pictures < 10; nbr_cropped_pictures++) {
					Mat src_gray_crop; //matrice of the cropped image
									   ///dimension of the cropped image
					srandomNum = rand();
					int rect_width = rand() % (96 - 10) + 10;
					int rect_height = rand() % (96 - 10) + 10;
					int rect_x = rand() % (96 - rect_width);
					int rect_y = rand() % (96 - rect_height);
					Rect RectangleToCrop(rect_x, rect_y, rect_width, rect_height); // object rect used to crop
					src_gray_crop = src_gray_resized(RectangleToCrop).clone(); // image is cropped
																			   ///Resize the cropped image to the size of the other image
					Mat src_gray_crop_resized;
					cv::resize(src_gray_crop, src_gray_crop_resized, Size(120, 120), 0, 0, INTER_AREA);

					HOGDescriptor hog(
						src_gray_resized.size(),//Size(20, 20), //winSize
						Size(20, 20), //blocksize
						Size(10, 10), //blockStride,
						Size(10, 10), //cellSize,
						9, //nbins,
						1, //derivAper,
						-1, //winSigma,
						0, //histogramNormType,
						0.2, //L2HysThresh,
						0,//gammal correction,
						64,//nlevels=64
						1);
					hog.compute(src_gray_crop_resized, descriptors, Size(136, 136), Size(8, 8));
					//visualizeHOG(src_gray_resized, descriptors, hog, 6);
					//waitKey(0);
					Mat1f m1(1, descriptors.size(), descriptors.data());
					features[image_cat].push_back(m1);
					labels.push_back(image_cat);
					/// For each cropped picture that has been resized, we also get the hog descriptor for when it is rotated
					if (rotateOn == 1) {
						Mat src_gray_resized__cropped_rotated;
						for (int cnt_rotation = 0; cnt_rotation < 3; cnt_rotation++) {
							cv::rotate(src_gray_crop_resized, src_gray_resized__cropped_rotated, 0);//last parameter is Rotate_flag, if 0, 90degre clockwise
							HOGDescriptor hog(
								src_gray_resized.size(),//Size(20, 20), //winSize
								Size(20, 20), //blocksize
								Size(10, 10), //blockStride,
								Size(10, 10), //cellSize,
								9, //nbins,
								1, //derivAper,
								-1, //winSigma,
								0, //histogramNormType,
								0.2, //L2HysThresh,
								0,//gammal correction,
								64,//nlevels=64
								1);
							hog.compute(src_gray_resized__cropped_rotated, descriptors, Size(136, 136), Size(8, 8));
							//visualizeHOG(src_gray_resized, descriptors, hog, 6);
							//waitKey(0);
							Mat1f m1(1, descriptors.size(), descriptors.data());
							features[image_cat].push_back(m1);
							labels.push_back(image_cat);

							///cropped->resized->rotated->blurred
							if (blurOn == 1) {
								for (int sigma = 1; sigma < 6; sigma = sigma + 2) {
									Mat src_gray_resized_cropped_rotated_blurred;
									///Apply some blur 
									GaussianBlur(src_gray_resized__cropped_rotated, src_gray_resized_cropped_rotated_blurred, Size(0, 0), sigma, 0, BORDER_DEFAULT);//when Size(0,0), kernel is computed from sigma, if sigma.y=0,then its equal to sigma.x
									HOGDescriptor hog(
										src_gray_resized.size(),//Size(20, 20), //winSize
										Size(20, 20), //blocksize
										Size(10, 10), //blockStride,
										Size(10, 10), //cellSize,
										9, //nbins,
										1, //derivAper,
										-1, //winSigma,
										0, //histogramNormType,
										0.2, //L2HysThresh,
										0,//gammal correction,
										64,//nlevels=64
										1);

									hog.compute(src_gray_resized_cropped_rotated_blurred, descriptors, Size(136, 136), Size(8, 8));
									//visualizeHOG(src_gray_resized, descriptors, hog, 6);
									//waitKey(0);
									Mat1f m1(1, descriptors.size(), descriptors.data());
									features[image_cat].push_back(m1);
									labels.push_back(image_cat);
								}
							}
						}
					}

					///If we want extented datas through blurred images, blur each image with 3 lvl of blurring sigma = 1,3,5
					/// cropped->resized->blurred
					if (blurOn == 1) {
						for (int sigma = 1; sigma < 6; sigma = sigma + 2) {
							Mat src_gray_resized_cropped_blurred;
							///Apply some blur 
							GaussianBlur(src_gray_crop_resized, src_gray_resized_cropped_blurred, Size(0, 0), sigma, 0, BORDER_DEFAULT);//when Size(0,0), kernel is computed from sigma, if sigma.y=0,then its equal to sigma.x
							HOGDescriptor hog(
								src_gray_resized.size(),//Size(20, 20), //winSize
								Size(20, 20), //blocksize
								Size(10, 10), //blockStride,
								Size(10, 10), //cellSize,
								9, //nbins,
								1, //derivAper,
								-1, //winSigma,
								0, //histogramNormType,
								0.2, //L2HysThresh,
								0,//gammal correction,
								64,//nlevels=64
								1);

							hog.compute(src_gray_resized_cropped_blurred, descriptors, Size(136, 136), Size(8, 8));
							//visualizeHOG(src_gray_resized, descriptors, hog, 6);
							//waitKey(0);
							Mat1f m1(1, descriptors.size(), descriptors.data());
							features[image_cat].push_back(m1);
							labels.push_back(image_cat);
						}
					}
					srand((unsigned int)srandomNum);
				}
			}
		}
	}

	///Update the nbr of pictures per class so the minimum label for each class can be calcuted(needed in training phase) starting from this table
	for (int idx_class = 0; idx_class < number_of_cat; idx_class++) {
		nbr_pictures_cat[idx_class] = nbr_pictures_cat[idx_class] * (1 + (rotateOn * 3) + (blurOn * 3) + (cropOn * 50));
	}

}

int main(){
	///Variables(careful, not all of them)
	int number_of_cat = 6;
	//int nbr_pictures_cat[] = { 4, 5,4,5,7,5 };

	int nbr_pictures_cat[] = { 49, 66, 42, 53, 67, 110 };
	vector<Mat1f>  features(number_of_cat);
	Mat labels;

	vector<Mat1f> extended_features(number_of_cat);
	Mat extended_labels;

	/// Number of pictures
	///Final values
	//int number_of_cat = 6;
	//int nbr_pictures_cat[] = { 49, 66, 42, 53, 67, 110 };
	///Temp values


	///Choose your way of creating the set of data
	getInitialDataDescriptors(features, labels, number_of_cat, nbr_pictures_cat);
	//getExpandedDataDescriptors(extended_features, extended_labels, number_of_cat, nbr_pictures_cat, 1, 1, 1);
	

	///Forest creation
	int size_forest = 5;
	int CVFolds = 0;
	int MaxDepth = 20;
	int MinSample_Count = 5;
	int MaxCategories = 6;
	MyForest  testForest;
	testForest.create(size_forest, CVFolds, MaxDepth, MinSample_Count, MaxCategories);


	///Training phase
	testForest.train(features, labels, nbr_pictures_cat);
	//testForest.train(extended_features, extended_labels, nbr_pictures_cat);



	///Testing phase

	/// Beginning and end of path to pictures
	string schemetest = "C:/Users/Kamel GUERDA/Desktop/TDCV_exercice2/data/task2/test/";
	string extension = ".jpg";
	string path = schemetest + "00" + "/" + "0050" + extension;
	///example path="C:/Users/Kamel GUERDA/Desktop/TDCV_exercice2/data/task2/train/00/0000.jpg";
	Mat src;
	Mat src_gray;
	Mat src_gray_resized;
	vector<float> descriptors;

	src = imread(path);
	///check if picture exist
	if (!src.data)
	{
		return -1;
	}
	/// Convert it to gray
	cv::cvtColor(src, src_gray, CV_BGR2GRAY);
	///Scale to 120*120 image
	cv::resize(src_gray, src_gray_resized, Size(120, 120), 0, 0, INTER_AREA);
	///Compute HOG
	HOGDescriptor hog(
		src_gray_resized.size(),//Size(20, 20), //winSize
		Size(20, 20), //blocksize
		Size(10, 10), //blockStride,
		Size(10, 10), //cellSize,
		9, //nbins,
		1, //derivAper,
		-1, //winSigma,
		0, //histogramNormType,
		0.2, //L2HysThresh,
		0,//gammal correction,
		64,//nlevels=64
		1);
	hog.compute(src_gray_resized, descriptors, Size(136, 136), Size(8, 8));
	Mat1f m2(1, descriptors.size(), descriptors.data());

	///Prediction from one tree for one picture
	double * prediction;

	prediction = testForest.predict(descriptors);
	double class_predicted = prediction[0];
	double confidence = prediction[1];

	return 0;
}	

///Just a few line to check what goes wrong with one part of the program
/*	try {
testForest.train(features, labels, nbr_pictures_cat);
}
catch (const std::exception & e) {
cout << "Doesn't work : " << e.what();
}
*/

///Old way of defining our forest
/*///Create DTrees
Ptr<DTrees> myDTree[5];// test succeeded, possible to instantiate multiple DTrees
myDTree[0] = DTrees::create();

///Set some parameters of the 1st DTree	
myDTree[0]->setCVFolds(0); // the number of cross-validation folds
myDTree[0]->setMaxDepth(8);
myDTree[0]->setMinSampleCount(2);

*/

///Previous useless stuff used to learn
/*	/// Generate grad_x and grad_y
	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	/// Gradient X
	///Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
	Sobel(src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT);
	convertScaleAbs(grad_x, abs_grad_x);

	/// Gradient Y
	///Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
	Sobel(src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT);
	convertScaleAbs(grad_y, abs_grad_y);

	/// Total Gradient (approximate), need euclidian distance
	addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);
	grad = abs_grad_x*0.5 + abs_grad_y*0.5;
	/// Create window
	namedWindow(window_name, CV_WINDOW_AUTOSIZE);
	imshow(window_name, src_gray_resized);
	waitKey(0);
*/

