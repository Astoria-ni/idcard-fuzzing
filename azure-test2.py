import os.path
import requests
from azure.cognitiveservices.vision.computervision import ComputerVisionClient
from azure.cognitiveservices.vision.computervision.models import VisualFeatureTypes
from msrest.authentication import CognitiveServicesCredentials
from azure.cognitiveservices.vision.computervision.models import OperationStatusCodes

SUBSCRIPTION_KEY_ENV_NAME = "b58e1f5e03f14978b43ed16af440f653"
COMPUTERVISION_LOCATION = os.environ.get(
    "COMPUTERVISION_LOCATION", "westcentralus")

IMAGES_FOLDER = "/Users/jishnuroychoudhury/Desktop/Images_Database/POL/passport"
from PIL import Image

def image_analysis_in_stream(subscription_key):
    """ImageAnalysisInStream.
    This will analyze an image from a stream and return all available features.
    """
    client = ComputerVisionClient(
        endpoint="https://jishnu-roychoudhury-sgp.cognitiveservices.azure.com/",
        credentials=CognitiveServicesCredentials(subscription_key)
    )

    with open(os.path.join(IMAGES_FOLDER, "danl.png"), "rb") as image_stream:
        image_analysis = client.analyze_image_in_stream(
            image=image_stream,
            visual_features=[
                VisualFeatureTypes.image_type,  # Could use simple str "ImageType"
                VisualFeatureTypes.faces,      # Could use simple str "Faces"
                VisualFeatureTypes.categories,  # Could use simple str "Categories"
                VisualFeatureTypes.color,      # Could use simple str "Color"
                VisualFeatureTypes.tags,       # Could use simple str "Tags"
                VisualFeatureTypes.description  # Could use simple str "Description"
            ]
        )

    print("This image can be described as: {}\n".format(
        image_analysis.description.captions[0].text))

    print("Tags associated with this image:\nTag\t\tConfidence")
    for tag in image_analysis.tags:
        print("{}\t\t{}".format(tag.name, tag.confidence))

    print("\nThe primary colors of this image are: {}".format(
        image_analysis.color.dominant_colors))


def recognize_text(subscription_key):
    """RecognizeTextUsingRecognizeAPI.
    This will recognize text of the given image using the recognizeText API.
    """
    import time
    client = ComputerVisionClient(
        endpoint="https://jishnu-roychoudhury-sgp.cognitiveservices.azure.com/",
        credentials=CognitiveServicesCredentials(subscription_key)
    )

    with open(os.path.join(IMAGES_FOLDER, "danl.png"), "rb") as image_stream:
        job = client.recognize_text_in_stream(
            image=image_stream,
            mode="Printed",
            raw=True
        )
    operation_id = job.headers['Operation-Location'].split('/')[-1]

    image_analysis = client.get_text_operation_result(operation_id)
    while image_analysis.status in ['NotStarted', 'Running']:
        time.sleep(1)
        image_analysis = client.get_text_operation_result(
            operation_id=operation_id)

    print("Job completion is: {}\n".format(image_analysis.status))

    print("Recognized:\n")
    lines = image_analysis.recognition_result.lines
    print(lines[0].words[0].text)  # "make"
    print(lines[1].words[0].text)  # "things"
    print(lines[2].words[0].text)  # "happen"

import numpy as np
import cv2 as cv

def convert_from_cv2_to_image(img: np.ndarray) -> Image:
    return Image.fromarray(cv.cvtColor(img, cv.COLOR_BGR2RGB))
    #return Image.fromarray(img)

def solve(lines,some_filename):
    #image = Image.open(IMAGES_FOLDER+"/"+some_filename)
    #img = np.asarray(image)
    img = cv.imread(os.path.join(IMAGES_FOLDER, some_filename),cv.IMREAD_COLOR)


    for line in lines:
        #print(line["boundingBox"])
        arr = line["boundingBox"]
        cv.rectangle(img, (arr[0], arr[1]), (arr[4], arr[5]), (255, 255, 255), -1)
        cv.putText(img,line["text"],(arr[6],arr[7]),cv.FONT_HERSHEY_SIMPLEX,1,(0,0,0),2)
        #print(line["text"])

    smth = convert_from_cv2_to_image(img)
    smth.show()



def recognize_printed_text_in_stream(subscription_key, some_filename):
    """RecognizedPrintedTextUsingOCR_API.
    This will do an OCR analysis of the given image.
    """
    client = ComputerVisionClient(
        endpoint="https://jishnu-roychoudhury-sgp.cognitiveservices.azure.com/",
        credentials=CognitiveServicesCredentials(subscription_key)
    )

    with open(os.path.join(IMAGES_FOLDER, some_filename), "rb") as image_stream:
        read_response = client.read_in_stream(
            image=image_stream,
            raw=True
        )
    import time
    import json
    time.sleep(5)
    result_url = read_response.headers.get('Operation-Location')
    result = requests.get(result_url,headers = {"Ocp-Apim-Subscription-Key":subscription_key})
    todos = json.loads(result.text)

    res = todos["analyzeResult"]
    smth = res["readResults"]
    
    sm = smth[0]
    lines = sm["lines"]

    solve(lines,some_filename)

    #lines = image_analysis.regions[0].lines
    '''
    print("Recognized:\n")
    
    for region in image_analysis.regions:
        lines = region.lines
        for line in lines:
            line_text = " ".join([word.text for word in line.words])
            print(line_text)
    '''
    '''
    for line in lines:
        line_text = " ".join([word.text for word in line.words])
        print(line_text)
    '''


if __name__ == "__main__":
    import sys, os.path
    sys.path.append(os.path.abspath(os.path.join(__file__, "..", "..", "..")))
    
    directory = r'/Users/jishnuroychoudhury/Desktop/Images_Database/POL/passport'
    import os
    for filename in os.listdir(directory):
        if filename.endswith(".jpg") or filename.endswith(".png"):
            #print(os.path.join(directory, filename))
            recognize_printed_text_in_stream(SUBSCRIPTION_KEY_ENV_NAME,filename)
        else:
            continue