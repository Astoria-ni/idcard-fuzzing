#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <filesystem>
#include "bits/stdc++.h"
using recursive_directory_iterator = std::__fs::filesystem::recursive_directory_iterator;
using namespace cv;
using namespace std;

#define MAGIC 15

typedef struct t_color_node {
    cv::Mat       mean;       // The mean of this node
    cv::Mat       cov;
    uchar         classid;    // The class ID

    t_color_node  *left;
    t_color_node  *right;
} t_color_node;

cv::Mat get_dominant_palette(std::vector<cv::Vec3b> colors) {
    const int tile_size = 64;
    cv::Mat ret = cv::Mat(tile_size, tile_size*colors.size(), CV_8UC3, cv::Scalar(0));

    for(int i=0;i<colors.size();i++) {
        cv::Rect rect(i*tile_size, 0, tile_size, tile_size);
        cv::rectangle(ret, rect, cv::Scalar(colors[i][0], colors[i][1], colors[i][2]), FILLED);
    }

    return ret;
}

std::vector<t_color_node*> get_leaves(t_color_node *root) {
    std::vector<t_color_node*> ret;
    std::queue<t_color_node*> queue;
    queue.push(root);

    while(queue.size() > 0) {
        t_color_node *current = queue.front();
        queue.pop();

        if(current->left && current->right) {
            queue.push(current->left);
            queue.push(current->right);
            continue;
        }

        ret.push_back(current);
    }

    return ret;
}

std::vector<cv::Vec3b> get_dominant_colors(t_color_node *root) {
    std::vector<t_color_node*> leaves = get_leaves(root);
    std::vector<cv::Vec3b> ret;

    for(int i=0;i<leaves.size();i++) {
        cv::Mat mean = leaves[i]->mean;
        ret.push_back(cv::Vec3b(mean.at<double>(0)*255.0f,
                                mean.at<double>(1)*255.0f,
                                mean.at<double>(2)*255.0f));
    }

    return ret;
}

int get_next_classid(t_color_node *root) {
    int maxid = 0;
    std::queue<t_color_node*> queue;
    queue.push(root);

    while(queue.size() > 0) {
        t_color_node* current = queue.front();
        queue.pop();

        if(current->classid > maxid)
            maxid = current->classid;

        if(current->left != NULL)
            queue.push(current->left);

        if(current->right)
            queue.push(current->right);
    }

    return maxid + 1;
}

void get_class_mean_cov(cv::Mat img, cv::Mat classes, t_color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const uchar classid = node->classid;

    cv::Mat mean = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
    cv::Mat cov = cv::Mat(3, 3, CV_64FC1, cv::Scalar(0));

    // We start out with the average color
    double pixcount = 0;
    for(int y=0;y<height;y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for(int x=0;x<width;x++) {
            if(ptrClass[x] != classid)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
            scaled.at<double>(0) = color[0]/255.0f;
            scaled.at<double>(1) = color[1]/255.0f;
            scaled.at<double>(2) = color[2]/255.0f;

            mean += scaled;
            cov = cov + (scaled * scaled.t());

            pixcount++;
        }
    }

    cov = cov - (mean * mean.t()) / pixcount;
    mean = mean / pixcount;

    // The node mean and covariance
    node->mean = mean.clone();
    node->cov = cov.clone();

    return;
}

void partition_class(cv::Mat img, cv::Mat classes, uchar nextid, t_color_node *node) {
    const int width = img.cols;
    const int height = img.rows;
    const int classid = node->classid;

    const uchar newidleft = nextid;
    const uchar newidright = nextid+1;

    cv::Mat mean = node->mean;
    cv::Mat cov = node->cov;
    cv::Mat eigenvalues, eigenvectors;
    cv::eigen(cov, eigenvalues, eigenvectors);

    cv::Mat eig = eigenvectors.row(0);
    cv::Mat comparison_value = eig * mean;

    node->left = new t_color_node();
    node->right = new t_color_node();

    node->left->classid = newidleft;
    node->right->classid = newidright;

    // We start out with the average color
    for(int y=0;y<height;y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for(int x=0;x<width;x++) {
            if(ptrClass[x] != classid)
                continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1,
                                  CV_64FC1,
                                  cv::Scalar(0));

            scaled.at<double>(0) = color[0]/255.0f;
            scaled.at<double>(1) = color[1]/255.0f;
            scaled.at<double>(2) = color[2]/255.0f;

            cv::Mat this_value = eig * scaled;

            if(this_value.at<double>(0, 0) <= comparison_value.at<double>(0, 0)) {
                ptrClass[x] = newidleft;
            } else {
                ptrClass[x] = newidright;
            }
        }
    }
    return;
}

cv::Mat get_quantized_image(cv::Mat classes, t_color_node *root) {
    std::vector<t_color_node*> leaves = get_leaves(root);

    const int height = classes.rows;
    const int width = classes.cols;
    cv::Mat ret(height, width, CV_8UC3, cv::Scalar(0));

    for(int y=0;y<height;y++) {
        uchar *ptrClass = classes.ptr<uchar>(y);
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        for(int x=0;x<width;x++) {
            uchar pixel_class = ptrClass[x];
            for(int i=0;i<leaves.size();i++) {
                if(leaves[i]->classid == pixel_class) {
                    ptr[x] = cv::Vec3b(leaves[i]->mean.at<double>(0)*255,
                                       leaves[i]->mean.at<double>(1)*255,
                                       leaves[i]->mean.at<double>(2)*255);
                }
            }
        }
    }

    return ret;
}

cv::Mat get_viewable_image(cv::Mat classes) {
    const int height = classes.rows;
    const int width = classes.cols;

    const int max_color_count = 12;
    cv::Vec3b *palette = new cv::Vec3b[max_color_count];
    palette[0]  = cv::Vec3b(  0,   0,   0);
    palette[1]  = cv::Vec3b(255,   0,   0);
    palette[2]  = cv::Vec3b(  0, 255,   0);
    palette[3]  = cv::Vec3b(  0,   0, 255);
    palette[4]  = cv::Vec3b(255, 255,   0);
    palette[5]  = cv::Vec3b(  0, 255, 255);
    palette[6]  = cv::Vec3b(255,   0, 255);
    palette[7]  = cv::Vec3b(128, 128, 128);
    palette[8]  = cv::Vec3b(128, 255, 128);
    palette[9]  = cv::Vec3b( 32,  32,  32);
    palette[10] = cv::Vec3b(255, 128, 128);
    palette[11] = cv::Vec3b(128, 128, 255);

    cv::Mat ret = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));
    for(int y=0;y<height;y++) {
        cv::Vec3b *ptr = ret.ptr<cv::Vec3b>(y);
        uchar *ptrClass = classes.ptr<uchar>(y);
        for(int x=0;x<width;x++) {
            int color = ptrClass[x];
            if(color >= max_color_count) {
                printf("You should increase the number of predefined colors!\n");
                continue;
            }
            ptr[x] = palette[color];
        }
    }

    return ret;
}

t_color_node* get_max_eigenvalue_node(t_color_node *current) {
    double max_eigen = -1;
    cv::Mat eigenvalues, eigenvectors;

    std::queue<t_color_node*> queue;
    queue.push(current);

    t_color_node *ret = current;
    if(!current->left && !current->right)
        return current;

    while(queue.size() > 0) {
        t_color_node *node = queue.front();
        queue.pop();

        if(node->left && node->right) {
            queue.push(node->left);
            queue.push(node->right);
            continue;
        }

        cv::eigen(node->cov, eigenvalues, eigenvectors);
        double val = eigenvalues.at<double>(0);
        if(val > max_eigen) {
            max_eigen = val;
            ret = node;
        }
    }

    return ret;
}

std::vector<cv::Vec3b> find_dominant_colors(cv::Mat img, int count) {
    const int width = img.cols;
    const int height = img.rows;

    cv::Mat classes = cv::Mat(height, width, CV_8UC1, cv::Scalar(1));
    t_color_node *root = new t_color_node();

    root->classid = 1;
    root->left = NULL;
    root->right = NULL;

    t_color_node *next = root;
    get_class_mean_cov(img, classes, root);
    for(int i=0;i<count-1;i++) {
        next = get_max_eigenvalue_node(root);
        partition_class(img, classes, get_next_classid(root), next);
        get_class_mean_cov(img, classes, next->left);
        get_class_mean_cov(img, classes, next->right);
    }

    std::vector<cv::Vec3b> colors = get_dominant_colors(root);

    cv::Mat quantized = get_quantized_image(classes, root);
    cv::Mat viewable = get_viewable_image(classes);
    cv::Mat dom = get_dominant_palette(colors);

    cv::imwrite("./classification.png", viewable);
    cv::imwrite("./quantized.png", quantized);
    cv::imwrite("./palette.png", dom);

    return colors;
}

void sol(std::string image_path, int field)
{
	
    //std::string image_path = "/Users/jishnuroychoudhury/Desktop/cia.png";

    Mat img = imread(image_path, IMREAD_COLOR);
    
    const char *path = image_path.c_str();
	
	//Pix *image = pixRead("/Users/jishnuroychoudhury/Desktop/cia.png");
	Pix *image = pixRead(path);
	tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
	//api->Init(NULL, "eng", tesseract::OEM_TESSERACT_ONLY);
	api->Init(NULL, "eng", tesseract::OEM_DEFAULT);
	api->SetImage(image);
	api->SetPageSegMode(tesseract::PSM_SPARSE_TEXT);
	api->Recognize(0);
	tesseract::ResultIterator* ri = api->GetIterator();
	tesseract::PageIteratorLevel level = tesseract::RIL_BLOCK;
	if (ri != 0) {
		
		int ct = 0;
		do {
			
			ct++;
			
		  const char* word = ri->GetUTF8Text(level);
		  
		  std::string wordstring(word);
		  
		  if(wordstring.size() >= 2){
			wordstring.pop_back(); wordstring.pop_back();
	      }
		  
		  float conf = ri->Confidence(level);
		  
		  if(conf <= 70.00) continue;
		  
		  /*const char *font_name;
		  bool bold, italic, underlined, monospace, serif, smallcaps;
		  int pointsize, font_id;
		  font_name = ri->WordFontAttributes(&bold, &italic, &underlined,
												   &monospace, &serif, &smallcaps,
												   &pointsize, &font_id);
		  */
		  int x1, y1, x2, y2;
		  ri->BoundingBox(level, &x1, &y1, &x2, &y2);
		  
		  //bool cond = (wordstring=="Henderson" || wordstring=="Elizabeth");
		  bool cond = 1;
		  
		  //if(cond) rectangle(img, Point(x1,y1), Point(x2,y2), Scalar(255,255,255), FILLED, LINE_8); //draw white rectangle
		  
		  //if(cond) putText(img, wordstring, Point(x1,y2), FONT_HERSHEY_DUPLEX, 1, Scalar(70,70,70), 2);
		  
		  printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n",
				   word, conf, x1, y1, x2, y2); 
				   //font_id can be printed but it's currently not looked for.
			
			//get dominant color
			
			cv::Mat BB = img(cv::Rect(x1,y1,x2-x1+1,y2-y1+1));

			vector<cv::Vec3b> dom_color = find_dominant_colors(BB,1); //gets us our dominant color
			//assert(dom_color.size()==1);
			int rr = dom_color[0].val[0];
			int gg = dom_color[0].val[1];
			int bb = dom_color[0].val[2];
			
			cout<<rr<<' '<<gg<<' '<<bb<<endl;
			
			if(cond) rectangle(img, Point(x1,y1), Point(x2,y2), Scalar(rr,gg,bb), FILLED, LINE_8); //draw color rect
			if(cond) putText(img, wordstring, Point(x1,y2), FONT_HERSHEY_DUPLEX, 1, Scalar(70,70,70), 2);
			
		  delete[] word;
		} while (ri->Next(level));
	}
	api->End();
	
    if(img.empty())
    {
        std::cout << "Could not read the image: " << image_path << std::endl;
        //return 1;
        return;
    }
    
    
    
    //std::cout<<"HIHI"<<std::endl;
    
    imshow("Display window", img);
    int k = waitKey(0); // Wait for a keystroke in the window
    /*if(k == 's')
    {
        imwrite("starry_night.png", img);
    }*/
    //return 0;
}

int main(){
	std::string myPath = "/Users/jishnuroychoudhury/Desktop/Images_Database/POL/results";
	for (const auto& dirEntry : recursive_directory_iterator(myPath))
		sol(dirEntry.path(),MAGIC);
	//sol("/Users/jishnuroychoudhury/Downloads/driving-licenses-500x500.png",MAGIC);
}

