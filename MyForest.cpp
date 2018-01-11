#include "stdafx.h"
#include "MyForest.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/ml.hpp>
#include "opencv2/core/mat.hpp"
#include <stdlib.h> // srand, rand 
#include <stdio.h>
#include <math.h>
#include <string> 
#include <time.h>       // time 
#include <random>
#include <ctime> 
#include <cstdlib>
#include <tuple>
#include <iostream>
using namespace std;
using namespace cv;
using namespace cv::ml;


using namespace cv;
using namespace std;



///Constructor
MyForest::MyForest() {
	Ptr<DTrees> myDTree[100];
	size_forest = 10;
	CVFolds = 0;
	MaxDepth = 20;
	MinSample_Count = 5; 
	MaxCategories = 1;
	for (int i = 0; i < size_forest; i++) {
		myDTree[i] = DTrees::create();
		myDTree[i]->setCVFolds(0); // the number of cross-validation folds
		myDTree[i]->setMaxDepth(8);
		myDTree[i]->setMinSampleCount(2);
	}
}


///Forest creattion
/// parameters are the number of tree in the forest and the parameters that describe each tree
void MyForest::create(int p_size_forest, int p_CVFolds, int p_MaxDepth, int p_MinSample_Count, int p_MaxCategories) {
	size_forest = p_size_forest;
	CVFolds = p_CVFolds;
	MaxDepth = p_MaxDepth;
	MinSample_Count = p_MinSample_Count;
	MaxCategories = p_MaxCategories;
	for (int tree_idx = 0; tree_idx < size_forest; tree_idx++) {
		myDTree[tree_idx] = DTrees::create();
		myDTree[tree_idx]->setCVFolds(CVFolds); // the number of cross-validation folds
		myDTree[tree_idx]->setMaxDepth(MaxDepth);
		myDTree[tree_idx]->setMinSampleCount(MinSample_Count);
		myDTree[tree_idx]->setMaxCategories(MaxCategories);
	}
	int tot_classes = MaxCategories;
}

///Train Forest: each Tree is trained with a random part of the complete input data
void MyForest::train(vector<Mat1f> label_per_feats, Mat labels, int size_samples__per_class[]) {
	// for task 3, either change 6 to number of class, or use new, delete and a variable as a parameter for the number of class, don't forget to change MyForest.h if paramaters are changed
	//int number_of_class; uncomment line 62 63 139 and put line 61 in parameters
	//int* starting_index_class = new int[number_of_class];
	//std::memset(starting_index_class, 0, sizeof(starting_index_class))
	//int size_samples__per_class[] = {49, 66, 42, 53, 67, 110};//Number of pictures per class // This should be put as a comments or delete parameter
	std::vector<int> starting_index_class(MaxCategories,0); 
	for (int i = 0; i < 6; i++) {
		if (i == 0) {
			starting_index_class[i] = size_samples__per_class[i];
		}
		else {
			starting_index_class[i] = size_samples__per_class[i] + starting_index_class[i-1];
		}
	}

	int min_label = size_samples__per_class[0];
	for (int class_idx = 1; class_idx < MaxCategories; class_idx++)
	{
		if (size_samples__per_class[class_idx] < min_label)
			min_label = size_samples__per_class[class_idx];
	}
	//cout<<"\nMinimum label value" <<min_label;
	//end of code to find the minimum number of elements in array


	//gng to generate random subset for each trees
	cout << "\n\ngng to generate the random train set for the forest ";
	vector<Mat1f> feat_trainset_per_tree(size_forest);
	vector<Mat> labels_trainset_per_tree(size_forest);

	time_t seconds;
	time(&seconds);
	int srandomNum;
	//cout << "\nseconds" << seconds;
	srand((unsigned int)seconds);
	for (int curr_tree = 0; curr_tree <size_forest; curr_tree++)
	{
		int min = 0;
		int max;
		cout << "\n\nGenerate the random train set for the tree " << curr_tree;
		srandomNum = rand() % ((unsigned int)seconds - min + 1) + min;
		//below for loop iterates through all the seperated descriptor classes based on labels and generate
		//random index and add it to particular sizeof(size_samples__per_class) / sizeof(*size_samples__per_class)
		for (int curr_class = 0; curr_class < MaxCategories; curr_class++)
		{
			max = size_samples__per_class[curr_class] - 1;
			std::vector<int> check_duplicate(max +1, 0);
			//int* check_duplicate = new int[max+1];
			//std::memset(check_duplicate, 0, sizeof(check_duplicate));
			int randomNum;
			cout << "\n random idx from clas " << curr_class << "\n";
			for (int i = 0; i<min_label; i++)
			{
				randomNum = rand() % (max - min + 1) + min;
				//cout << randomNum << ",";
				if (check_duplicate[randomNum] != 0) {
					i--;
				}
				if (check_duplicate[randomNum] == 0) {
					check_duplicate[randomNum]++;
					cout << randomNum << ",";
					//Mat temp_feat = label_per_feats[curr_class].row(randomNum);
					feat_trainset_per_tree[curr_tree].push_back(label_per_feats[curr_class].row(randomNum));

					//feat_trainset_per_tree[curr_tree].push_back(temp_feat);
					labels_trainset_per_tree[curr_tree].push_back(curr_class);
				}

			}
			//delete[] check_duplicate;
		}
		srand((unsigned int)srandomNum);
	}

	//gng to iterate through all the trees
	for (int idx = 0; idx <size_forest; idx++)
	{
		std::cout << "\n\ntraining tree " << idx;
		//code pending to create random subsets
		myDTree[idx]->train(cv::ml::TrainData::create(feat_trainset_per_tree[idx], cv::ml::ROW_SAMPLE, labels_trainset_per_tree[idx]));
	}


	//delete[] starting_index_class;
}

///Predict a class for a data : input should be the HOG descriptor
double * MyForest::predict(vector<float> test_descriptors) {
	int curr_predictd;
	Mat1f predictd_array;
	int* predictd_class = new int[MaxCategories];
	std::memset(predictd_class, 0, sizeof(predictd_class));
	//predictd_class is used the number of times each class is predicted by the trees in- 
	//- the forest

	for (int tree_idx = 0; tree_idx <size_forest; tree_idx++)
	{
		curr_predictd = myDTree[tree_idx]->predict(test_descriptors);
		predictd_class[curr_predictd]++;
		//cout<<"\npredicted class after updating "<< predictd_class[curr_predictd];                                           
	}
	///Determination of the class that has been predicted the most by trees
	int max_predicted_class = MaxCategories + 1;//assign a class/label outside of the catrgory to debug -
											  //incase of error
	for (int class_idx = 0; class_idx < (MaxCategories - 1); class_idx++)
	{
		if (predictd_class[class_idx]<predictd_class[class_idx + 1])
			max_predicted_class = class_idx + 1;
		else if (predictd_class[class_idx]>predictd_class[class_idx + 1])
			max_predicted_class = class_idx;

	}
	std::cout << "\nthe maximum predicted class by forest is  " << max_predicted_class;

	float confidence = ((float)predictd_class[max_predicted_class] / (float)size_forest) * 100;
	std::cout << "\nthe confidence of the forest in predicting class is " << confidence;
	//end of the code to find the class which is predicted maximum times
	delete[] predictd_class;
	double output[] = {max_predicted_class,confidence};
	return output;
}
