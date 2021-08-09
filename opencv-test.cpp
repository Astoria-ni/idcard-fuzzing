#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <filesystem>
using recursive_directory_iterator = std::__fs::filesystem::recursive_directory_iterator;
using namespace cv;
using namespace std;

#define MAGIC 11

//RE: just find the field in the schema
//

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
		  
		  /*const char *font_name;
		  bool bold, italic, underlined, monospace, serif, smallcaps;
		  int pointsize, font_id;
		  font_name = ri->WordFontAttributes(&bold, &italic, &underlined,
												   &monospace, &serif, &smallcaps,
												   &pointsize, &font_id);
		  */
		  int x1, y1, x2, y2;
		  ri->BoundingBox(level, &x1, &y1, &x2, &y2);
		  
		  bool cond = (wordstring=="Henderson" || wordstring=="Elizabeth");
		  
		  if(cond) rectangle(img, Point(x1,y1), Point(x2,y2), Scalar(255,255,255), FILLED, LINE_8); //draw white rectangle
		  
		  if(cond) putText(img, "DEBOWSKI", Point(x1,y2), FONT_HERSHEY_DUPLEX, 1, Scalar(70,70,70), 2);
		  
		  printf("word: '%s';  \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n",
				   word, conf, x1, y1, x2, y2); 
				   //font_id can be printed but it's currently not looked for.
				   
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
