#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/dpm.hpp>
#include <opencv2/tracking.hpp>

#include <cstring>
#include <fstream>
#include <iostream>
#include <stdio.h>

using namespace cv;
using namespace cv::dpm;
using namespace std;

void drawDPMBoxes(Mat &frame,
                  vector<DPMDetector::ObjectDetection> ds,
                  Scalar color,
                  string text);

void drawKCFBoxes(Mat &frame,
                  Rect rect,
                  Scalar color,
                  string text);

bool write_count(int count);

static const int LOW_PASS = 0;
static const string CLEAR_FILE = "/opt/mz-bicycle-commuter/reset";
static const string LEFT_OUTPUT_FILE = "/opt/mz-bicycle-commuter/left.count";

int main( int argc, char** argv )
{
    bool ENTER_LEAVE = true;
    bool FRAMES_FIRST = true;

    Mat frame;
    Mat image;
    const char* keys =
    {
        "{@model_path    | | Path of the DPM cascade model}"
        "{@video_source    | | Video Source}"
    };

    CommandLineParser parser(argc, argv, keys);
    string model_path(parser.get<string>(0));
    bool show_image (parser.get<bool>(1));

    // init DPM
    Ptr<DPMDetector> detector = \
    DPMDetector::create(vector<string>(1, model_path));
    vector<DPMDetector::ObjectDetection> ds;

    // init KCF
    Ptr<Tracker> tracker;
    Rect2d roi;
    Rect result; // Tracker results
    int count = 0;
    //use web camera
    VideoCapture capture(0);
    capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
    capture.set(CV_CAP_PROP_CONVERT_RGB, false);
    double fps = capture.get(CAP_PROP_FPS);

     cout << fps << endl;

    if( model_path.empty() )
    {
        cerr << "Fail to open model file!" << endl;
        return -1;
    }

    if ( !capture.isOpened() )
    {
        cerr << "Fail to open default camera (0)!" << endl;
        return -1;
    }

    // create window
    if (show_image) {
        namedWindow("tracker", 1);
    }

    while (true){
        //get the frame
        capture >> frame;

        if ( frame.empty() ){
            cerr << "Fail to get video!" << endl;
            break;
        }

        //DPM detection
        if( ENTER_LEAVE )
        {
            double t = (double) getTickCount(); //start time

            frame.copyTo(image);

            // detection
            detector->detect(image, ds);

            ds.erase(std::remove_if(ds.begin(),
                                    ds.end(),
                                    [&](const DPMDetector::ObjectDetection& o)
                                    {return o.score<LOW_PASS;}
                                    ),
                     ds.end());
            if (!ds.empty())
            {
                for (const auto i: ds)
                    std::cout << i.score << ' ';
                cout << endl;

                ENTER_LEAVE = false;
                FRAMES_FIRST = true;

            }

            t = ((double) getTickCount() - t)/getTickFrequency(); //elapsed time
            string text = format("%0.1f fps", 1.0/t); //calculate fps

            // draw the tracked object
            Scalar color(0,0,255);
            drawDPMBoxes(frame, ds, color, text);
        } else {

            //KCF tracking
            double t = (double) getTickCount(); //start time
            if ( FRAMES_FIRST )
            {
                // get bounding box
                roi = ds[0].rect;

                // initialize the tracker
                tracker = Tracker::create("KCF");
                bool success = tracker->init(frame,roi);

                FRAMES_FIRST = !success ;
            }

            // Update
            else{

                // update the tracking result
                bool was_successful = tracker->update(frame,roi);

                if (!was_successful) {
                    continue;
                }
                result = roi;
                //DPM model detect UAV's leave
                int x_value = 640 - result.x + result.width/2;
                int y_value = 480 - result.y + result.height/2;
                if (!(result.x > 0 && result.y > 0 && x_value > 0 && y_value > 0)){
                    ENTER_LEAVE = true;
                    count++;
                    bool reset_count = write_count(count);
                     string count_text;
                    if (reset_count) {
                        count = 0;
                        count_text = "Count was reset";
                    } else {
                        count_text  = format("%ld bicycles!", count);
                    }

                    cout << count_text << endl;

                }
            }
            t = ((double) getTickCount() - t)/getTickFrequency(); //elapsed time
            string text = format("%0.1f fps", 1.0/t); //calculate fps

            // draw the tracked object
            Scalar color(0,0,255);
            drawKCFBoxes(frame, roi, color, text);
        }

        // show image with the tracked object
        if(show_image) {
            imshow("tracker",frame);
        }
        if(waitKey(1)==27)break;
    }
    return 0;
}

bool write_count(int count)
{
    ofstream resultsFile;
    bool should_clear = std::ifstream(CLEAR_FILE).good();

    int file_count = should_clear ? 1 : count;
    resultsFile.open(LEFT_OUTPUT_FILE);
    resultsFile << file_count  << endl;
    resultsFile.close();
    if(should_clear) {
        remove(CLEAR_FILE.c_str());
    }

    return should_clear;
}

void drawDPMBoxes(Mat &frame,
                  vector<DPMDetector::ObjectDetection> ds,
                  Scalar color,
                  string text)
{
    for (unsigned int i = 0; i < ds.size(); i++)
    {
        rectangle(frame, ds[i].rect, color, 2);
    }

    // draw text on image
    Scalar textColor(255,0,0);
    putText(frame, text, Point(10,50), FONT_HERSHEY_PLAIN, 2, textColor, 2);
}

void drawKCFBoxes(Mat &frame,
                  Rect rect,
                  Scalar color,
                  string text)
{
    rectangle(frame, rect, color, 2);

    // draw text on image
    Scalar textColor(255,0,0);
    putText(frame, text, Point(10,50), FONT_HERSHEY_PLAIN, 2, textColor, 2);
}
