# idcard-fuzzing
Produce ID card modifications for use in testing.

Current list of files:

`azure-modif.py`: Code linked to Microsoft Azure for making modifications. Currently has slightly lower-quality modifications than Tesseract/C++ code. (`azure-test2.py` locally)

`levenhstein-testing.cpp`: Computes average Levenhstein distance to assess similarity of output between two runs. (`levenhstein-testing.cpp` locally)

`tess-dominant-transparent.cpp`: Code linked to Tesseract for making modifications. Uses hierarchical quantization with eigenvectors to find dominant color, and has experimental transparency (see constant ALPHA) to improve blend. Currently, transparency improves look of results to humans but confuses machines as original text remains. (`opencv-dom-transparency.cpp` locally)

`tess-dominant.cpp`: Code linked to Tesseract for making modifications. Uses hierarchical quantization with eigenvectors to find dominant color. Does not include experimental transparency. TBD: transfer parts of this code to Python/Azure. (`opencv-dom.cpp` locally)

`tess-white.cpp`: Code linked to Tesseract for making modifications. Simply replaces bounding box with white background. (`opencv-test.cpp` locally)
