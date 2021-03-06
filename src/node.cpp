#include "node.h"
#include "names.h"
//#include "boost\thread\mutex.hpp"
// Include for the apriltags
//#include "/apriltag2/apriltag.h"
#include <tag36h11.h>
#include <tag36h10.h>
#include <tag36artoolkit.h>
#include <tag25h9.h>
#include <tag25h7.h>
#include <tag16h5.h>
#include "apriltag2/AprilTagDetection.h"
#include "apriltag2/AprilTagDetectionArray.h"


// Namespaces
using namespace std;
using namespace cv;
using namespace FlyCapture2;

void imageCallback(const sensor_msgs::ImageConstPtr& msg)
{
    try
    {
        //        cv::imshow("view", cv_bridge::toCvShare(msg, "bgr8")->image);
        //        cv::waitKey(30);
        //        cout << "In the imageCallback"<<endl;
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("Could not convert from '%s' to 'bgr8'.", msg->encoding.c_str());
    }
}





namespace apriltag2_detector_ros 
{

    void Node::rotationMatrixZ(double alpha, cv::Mat_<float> (&R_z_90))
    {
        double pi = 3.14159265359;
        double alpha_rad = alpha*pi/180;
        //    cv::Mat_<float> RotZ(3,3);
        R_z_90 << cos(alpha_rad), -sin(alpha_rad), 0.0, sin(alpha_rad), cos(alpha_rad), 0.0, 0.0, 0.0, 1.0;

        //    R_z_90 = RotZ;
    }

    void Node::computeCog(double p[4][2], double (&returnArray)[2])
    {

        int cog_x, cog_y, temp1, temp2;

        for (int var = 0; var < 4; ++var) {
            temp1 = temp1 + p[var][0];
            temp2 = temp2 + p[var][1];
        }

        cog_x = temp1/4;
        cog_y = temp2/4;

        returnArray[0] = cog_x;
        returnArray[1] = cog_y;
    }

    //records last received image
    void Node::frameCallback(const sensor_msgs::ImageConstPtr& image, const sensor_msgs::CameraInfoConstPtr& cam_info)
    {
        boost::mutex::scoped_lock(lock_);
        image_header_ = image->header;
        try
        {
            cv_ptr = cv_bridge::toCvCopy(image, sensor_msgs::image_encodings::BGR8);
        }
        catch (cv_bridge::Exception& e)
        {
            ROS_ERROR("cv_bridge exception: %s", e.what());
            return;
        }
        //        ROS_INFO("Putting got_image_ to TRUE");
        // I_ = visp_bridge::toVispImageRGBa(*image); //make sure the image isn't worked on by locking a mutex
        //  cam_ = visp_bridge::toVispCameraParameters(*cam_info);

        got_image_ = true;
    }

    void Node::waitForImage()
    {
        while ( ros::ok ()){
            if(got_image_) return;
            ros::spinOnce();
        }

    }

    Node::Node():
        nh_("~"),
        queue_size_(1),
        cv_ptr(),
        cvI_(),
        image_header_(),
        got_image_(false),
        lastHeaderSeq_(0),
        first_time(false)
    {
        cout<<"-->In Node() constructor"<<endl;
    }

    void Node::spin(int argc, char** argv)
    {
        double pi = 3.14159265359;
        int camera_used = 2;
        //    string camera_info_topic_ = camera_info_topic;
        //    string image_topic_ = image_topic;
        //    if ( camera_used == 1 ) {
        //        cout << "camera_used = 1" <<endl;
        //        string camera_info_ = camera_info_topic;
        //        string image_topic_ = image_topic;
        //    } else if( camera_used == 2 ) {
        //        cout << "camera_used = 2" <<endl;
        //        string camera_info_topic_ = camera_info_topic_2;
        //        string image_topic_ = image_topic_2;
        //        cout<< "camera_image_topic: " <<  image_topic_2 << endl;
        //        cout<< "camera_info_topic: "  <<  camera_info_topic_2  << endl;
        //    }
        //    else {
        //        cout << "camera_used = DEFAULT = 1" <<endl;
        //        string camera_info_topic_ = camera_info_topic;
        //        string image_topic_ = image_topic;
        //    }
        //    string camera_info_topic_ = camera_info_topic;
        //    string image_topic_ = image_topic;
    //    message_filters::Subscriber<sensor_msgs::Image> raw_image_subscriber(nh_, image_topic, queue_size_);
    //    message_filters::Subscriber<sensor_msgs::CameraInfo> camera_info_subscriber(nh_, camera_info_topic, queue_size_);
        message_filters::Subscriber<sensor_msgs::Image> raw_image_subscriber(nh_, "/camera6_quadro6/image_mono", queue_size_);
        message_filters::Subscriber<sensor_msgs::CameraInfo> camera_info_subscriber(nh_, "/camera6_quadro6/camera_info", queue_size_);



        cout<< "camera_image_topic: " <<  image_topic << endl;
        cout<< "camera_info_topic: "  <<  camera_info_topic  << endl;
        message_filters::TimeSynchronizer<sensor_msgs::Image, sensor_msgs::CameraInfo> image_info_sync(raw_image_subscriber, camera_info_subscriber, queue_size_);
        image_info_sync.registerCallback(boost::bind(&Node::frameCallback,this, _1, _2));
        //    ros::Publisher chatter_pub = nh.advertise<std_msgs::String>("apriltag2_pose",1000);
        ros::Publisher detections_pub_ = nh_.advertise<apriltag2::AprilTagDetectionArray>("apriltag2_tag_detections", 1000);
        ros::Publisher detections_pub_1 = nh_.advertise<apriltag2::AprilTagDetection>("apriltag2_tag_detection1", 1000);
        ros::Publisher poseStamped_pub_1 = nh_.advertise<geometry_msgs::PoseStamped>("apriltag2_ID1_poseStamped", 1000);
        ros::Rate loop_rate(100);

        //    cv::namedWindow("view");
        //    cv::startWindowThread();
        //    image_transport::ImageTransport it(nh_);
        //    image_transport::Subscriber sub = it.subscribe("camera/image_raw", 1, imageCallback);
        //    cv::destroyWindow("view");

        //    sub.
        cout<<"OpenCV Version used:"<<CV_MAJOR_VERSION<<"."<<CV_MINOR_VERSION<<"."<<CV_SUBMINOR_VERSION<<endl;

        //wait for an image to be ready
        ROS_INFO("waiting for IMAGE...");
        waitForImage();
        {
            ROS_INFO("...Finished waiting for IMAGE!!");

            //when an image is ready ....
            boost::mutex::scoped_lock(lock_);
        }

        cout<<endl<<"Creating getopt"<<endl;
        getopt_t *getopt = getopt_create();

        getopt_add_bool(getopt, 'h', "help", 0, "Show this help");
        getopt_add_bool(getopt, 'd', "debug", 0, "Enable debugging output (slow)");
        getopt_add_bool(getopt, 'q', "quiet", 0, "Reduce output");
        getopt_add_string(getopt, 'f', "family", "tag36h11", "Tag family to use");
        getopt_add_int(getopt, '\0', "border", "1", "Set tag family border size");
        getopt_add_int(getopt, 't', "threads", "4", "Use this many CPU threads");
        getopt_add_double(getopt, 'x', "decimate", "1.0", "Decimate input image by this factor");
        cout<<"\nDecimation Changed\n";
        getopt_add_double(getopt, 'b', "blur", "0.0", "Apply low-pass blur to input");
        getopt_add_bool(getopt, '0', "refine-edges", 1, "Spend more time trying to align edges of tags");
        getopt_add_bool(getopt, '1', "refine-decode", 0, "Spend more time trying to decode tags");
        getopt_add_bool(getopt, '2', "refine-pose", 0, "Spend more time trying to precisely localize tags");

        if (!getopt_parse(getopt, argc, argv, 1) || getopt_get_bool(getopt, "help")) 
        {
            printf("Usage: %s [options]\n", argv[0]);
            getopt_do_usage(getopt);
            exit(0);
        }

        //////////////////////FlyCapture

        //    Error error;
        //    Camera camera;
        //    CameraInfo camInfo;


        //    // Connect the camera
        //    error = camera.Connect( 0 ); // Probably I need to change here if I have 2 cameras and I want to connect to a specific one.
        //    if ( error != PGRERROR_OK )
        //    {
        //        cout << "Failed to connect to camera" << endl;
        //        return false;
        //    }

        //    // Get the camera info and print it out
        //    error = camera.GetCameraInfo( &camInfo );
        //
        //    //		cout << "GetCameraInfo:" << error << endl;
        //    if ( error != PGRERROR_OK )
        //    {
        //        cout << "Failed to get camera info from camera" << endl;
        //        return false;
        //    }

        //    cout << "Starting to capture with the camera:" << endl;
        //    cout << "["<< camInfo.vendorName << " "
        //         << camInfo.modelName << " "
        //         << camInfo.serialNumber << "]" << std::endl;
        //    error = camera.StartCapture();

        /////////////////////////////////////
        // Initialize webcam laptop
        //	VideoCapture cap(0);
        //	if (!cap.isOpened()) {
        //		cerr << "Couldn't open video capture device" << endl;
        //		return -1;
        //	}
        //
        //	if ( error == PGRERROR_ISOCH_BANDWIDTH_EXCEEDED )
        //	{
        //		std::cout << "Bandwidth exceeded" << std::endl;
        //		return false;
        //	}
        //	else if ( error != PGRERROR_OK )
        //	{
        //		std::cout << "Failed to start image capture" << std::endl;
        //		return false;
        //	}
        //////////////////////////////////////
        char key = 0;

        // capture loop

        // Initialize tag detector with options
        apriltag_family_t *tf = NULL;
        const char *famname = getopt_get_string(getopt, "family");
        cout<<"FamilyName:"<<*famname<<endl;
        cout << "Creating apriltag_detector..." << endl;
        apriltag_detector_t *td = apriltag_detector_create();


        while(key != 'q' && ros::ok())
        {

            cout<<"Starting the APRILTAG LOOP"<<endl;
            // AprilTAG
            if (!strcmp(famname, "tag36h11"))
                tf = tag36h11_create();
            else if (!strcmp(famname, "tag36h10"))
                tf = tag36h10_create();
            else if (!strcmp(famname, "tag36artoolkit"))
                tf = tag36artoolkit_create();
            else if (!strcmp(famname, "tag25h9"))
                tf = tag25h9_create();
            else if (!strcmp(famname, "tag25h7"))
                tf = tag25h7_create();
            else if (!strcmp(famname, "tag16h5"))
            {
                cout << "-->creating tag16h5 family" << endl;
                tf = tag16h5_create();
                cout << "-->created tag16h5 family" << endl;
            }
            else 
            {
                printf("Unrecognized tag family name. Use e.g. \"tag36h11\".\n");
                exit(-1);
            }
            tf->black_border = getopt_get_int(getopt, "border");

            // apriltag_detector_t *td = apriltag_detector_create();
            apriltag_detector_add_family(td, tf);
            td->quad_decimate = getopt_get_double(getopt, "decimate");
            td->quad_sigma = getopt_get_double(getopt, "blur");
            td->nthreads = getopt_get_int(getopt, "threads");
            td->debug = getopt_get_bool(getopt, "debug");
            td->refine_edges = getopt_get_bool(getopt, "refine-edges");
            td->refine_decode = getopt_get_bool(getopt, "refine-decode");
            td->refine_pose = getopt_get_bool(getopt, "refine-pose");


            // In the following variable it will be put the coordinates of the center of the tag
            double cog [2];

            std_msgs::String message;
            std::stringstream ss1;
            ss1<<"helloworld ";
            message.data = ss1.str();
            //        ROS_INFO("%s",message.data.c_str());

            //        vpImage<unsigned char> I;
            //        vpImageIo::read( rawImage );
            //        std::vector<vpPoint> point(4);
            //        std::vector<vpDot2> dot(4);
            //        std::vector<vpImagePoint> corners(4);
            //        vpHomogeneousMatrix cMo;

            bool init = true;
            //        double tagsize = 0.167;
            //        double tagsize = 0.173;
            double tagsize = 0.162;

            Mat frame, gray;

            while (!key && ros::ok()) 
            {
                if (true)
                {
                    //                std::cout<<"-------------------->IN THE IF"<<endl;
                    // Get the image
                    //                Image rawImage;
                    //            Error error = camera.RetrieveBuffer( &rawImage );
                    //            if ( error != PGRERROR_OK )
                    //            {
                    //                std::cout << "capture error" << std::endl;
                    //                continue;
                    //            }

                    // convert to rgb
                    //            Image rgbImage;
                    //            rawImage.Convert( FlyCapture2::PIXEL_FORMAT_BGR, &rgbImage );

                    // convert to OpenCV Mat
                    //            unsigned int rowBytes = (double)rgbImage.GetReceivedDataSize()/(double)rgbImage.GetRows();
                    //            cv::Mat image = cv::Mat(rgbImage.GetRows(), rgbImage.GetCols(), CV_8UC3, rgbImage.GetData(),rowBytes);
                    boost::mutex::scoped_lock(lock_);
                    cv::Mat image = cv_ptr->image;
                    //            cap >> frame;

                    frame = image;
                    cv::cvtColor(frame, gray, 6);
                    //                cout << "frame.cols:" << frame.cols << endl; // This is the width of the image
                    //                cout << "frame.rows:" << frame.rows << endl; // This is the height of the image
                    //                cout << "frame.data:" << frame.data << endl;
                    // Make an image_u8_t header for the Mat data
                    image_u8_t im = { .width = gray.cols,
                                      .height = gray.rows,
                                      .stride = gray.cols,
                                      .buf = gray.data
                                    };

                    zarray_t *detections = apriltag_detector_detect(td, &im);
                    // cout << detections->data;
                    //                cout << zarray_size(detections) << " tags detected" << endl;
                    //                cout << "-----------------------" << endl;
                    // Draw detection outlines
                    int counter_3 = 0;
                    int counter_4 = 0;
                    for (int i = 0; i < zarray_size(detections); i++) 
                    {
                        apriltag_detection_t *det;
                        zarray_get(detections, i, &det);
                        if (det->id == 3) 
                        {
                            counter_3++;
                        }
                        else if(det->id == 4)
                        {
                            counter_4++;
                        }
                        line(frame, Point(det->p[0][0], det->p[0][1]),
                                Point(det->p[1][0], det->p[1][1]),
                                Scalar(0, 0xff, 0), 2);
                        line(frame, Point(det->p[0][0], det->p[0][1]),
                                Point(det->p[3][0], det->p[3][1]),
                                Scalar(0, 0, 0xff), 2);
                        line(frame, Point(det->p[1][0], det->p[1][1]),
                                Point(det->p[2][0], det->p[2][1]),
                                Scalar(0xff, 0, 0), 2);
                        line(frame, Point(det->p[2][0], det->p[2][1]),
                                Point(det->p[3][0], det->p[3][1]),
                                Scalar(0xff, 0, 0), 2);
                        //                    cout<<"["<<det->c[0]<<","<<det->c[1]<<"]"<<endl;
                        //                    cout << "Points: " << endl;
                        //                for (int var1 = 0; var1 < 4; ++var1) {
                        //                    cout << "Point: " << var1 <<endl;
                        //                    cout << ":::::( " << det->p[var1][0] <<","<<det->p[var1][1] << " ):::::"<<endl;
                        //                }
                        //                    if (det->id == 3) {
                        //                        cout<<"Points_3:" << endl;
                        //                        for (int var2 = 0; var2 < 4; ++var2) {
                        //                            cout << "("<< det->p[var2][0]<<","<<det->p[var2][1] << ")"<<endl;
                        //                        }
                        //                    }
                        Node::computeCog(det->p,cog);
                        //                    cout << "COG(ID_"<< det->id << "):"<< "[" <<cog[0] << "," << cog[1] << "]"<<endl;

                        double camera_matrix [9] = {342.420917, 0.000000, 551.137508, 0.000000, 338.672241, 372.152655, 0.000000, 0.000000, 1.000000};
                        //                    double camera_matrix [9] = {352.9714275312857, 0.0, 507.0838765900766, 0.0, 356.0344229270061, 387.8030061475569, 0.0, 0.0, 1.0};
                        double fx,fy,cx,cy;
                        fx=camera_matrix[0];
                        fy=camera_matrix[4];
                        cx=camera_matrix[2];
                        cy=camera_matrix[5];

                        //// Camera 15357017 parameters
                        const int npoints = 2; // number of point specified
                        // Points initialization
                        // Only 2 ponts in this example, in real code they are read from file.
                        Mat_<Point2f> points(1,1);
                        points(0) = Point2f(cog[0],cog[1]);
                        Mat_<float> camera_matrix_Mat_(3,3);
                        camera_matrix_Mat_ << 342.420917, 0.000000, 551.137508, 0.000000, 338.672241, 372.152655, 0.000000, 0.000000, 1.000000;
                        Mat_<float> distCoeffs(5,1);
                        distCoeffs << 0.011428, -0.000079, -0.002445, 0.013517, 0.000000;
                        Mat_<float> rectificationMatrix(3,3);
                        rectificationMatrix << 1.000000, 0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.000000, 1.000000;
                        //                    points(1) = Point2f(2560, 1920);
                        Mat dst;// leave empty, opencv will fill it.
                        // Undistorsion of the pixels we are interested in
                        cv::undistortPoints(points, dst, camera_matrix_Mat_, distCoeffs, rectificationMatrix);
                        //                    cout << "-->dst(ID_"<< det->id << "):" <<  dst <<endl;
                        Mat1f vectorRectified = (Mat1f(3,1) << dst.at<float>(0), dst.at<float>(1), 1.f);
                        //                    Mat_<float> vector1 = (camera_matrix_Mat_.inv() * vectorRectified);
                        cv::normalize(vectorRectified,vectorRectified);
                        //                    for (int var = 0; var < 3; ++var) {
                        //                        cout << "vectorRectified.at<float>("<<var<<"): " << vectorRectified.at<float>(var) << endl;
                        //                    }
                        //                    cout << "vectorRectified.at<float>(0): " << vectorRectified.at<float>(0) << endl;
                        //                    cout << "vectorRectified.at<float>(1): " << vectorRectified.at<float>(1) << endl;
                        //                    cout << "vectorRectified.at<float>(2): " << vectorRectified.at<float>(2) << endl;
                        //                    cout <<  "Norm of the vector: " << norm(vectorRectified) << endl;

                        // Rotation from b_ij in camera frame to b_ij in body
                        Mat_<float> R_y_90(3,3);
                        R_y_90 << 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0;
                        Mat_<float> R_x_90(3,3);
                        R_x_90 << 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0;
                        Mat_<float> R_z_90(3,3);
                        R_z_90 << 0.0, -1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0;

                        Mat_<float> R_y_m90(3,3);
                        R_y_m90 << 0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0;
                        Mat_<float> R_x_m90(3,3);
                        R_x_m90 << 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0;
                        Mat_<float> R_z_m90(3,3);
                        R_z_m90 << 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0;

                        //                    cout << "cos(60) "<< cos(60*pi/180) << endl;
                        //                    cout << "cos(60) "<< cos(60) << endl;
                        Mat1f vectorRectified_body;
                        //                    vectorRectified_body = R_x_90*R_z_90*vectorRectified;
                        Mat_<float> Rz(3,3);
                        rotationMatrixZ(-90, Rz);
                        vectorRectified_body = R_y_90*Rz*vectorRectified;
                        rotationMatrixZ(90, Rz);
                        vectorRectified_body = Rz*vectorRectified_body;
    //                    for (int var1 = 0; var1 < 3; ++var1) {
    //                        cout << "vectorRectified_body.at<float>("<<var1<<"): " << vectorRectified_body.at<float>(var1) << endl;
    //                    }
                        int fontface = FONT_HERSHEY_SCRIPT_SIMPLEX;

                        //                    ss << (std::string)vectorRectified_body.at<float>(var1);


                        stringstream stream;
                        stringstream stream_norm;
                        if (det->id == 3) 
                        {
                            stream << setprecision(4) << "bearing_i"<<det->id << "_" << counter_3 <<  "=[ " << vectorRectified_body.at<float>(0) << "," << vectorRectified_body.at<float>(1) << "," << vectorRectified_body.at<float>(2) << " ]";
                        } 
                        else if(det->id == 4)
                        {
                            stream << setprecision(4) << "bearing_i"<<det->id << "_" << counter_4 <<  "=[ " << vectorRectified_body.at<float>(0) << "," << vectorRectified_body.at<float>(1) << "," << vectorRectified_body.at<float>(2) << " ]";
                        }
                        //                    stream_norm << setprecision(4) << "Norm Bearing: " <<  norm(vectorRectified);

                        string s = stream.str();
                        string s1 = stream_norm.str();
                        if (det->id == 3) 
                        {
                            putText(frame,s,Point2f(25,700+30*(counter_3-1)), FONT_HERSHEY_PLAIN, 2,  Scalar(255,255,255));
                        } 
                        else if(det->id == 4)
                        {
                            putText(frame,s,Point2f(25,100+30*(counter_4-1)), FONT_HERSHEY_PLAIN, 2,  Scalar(255,255,255));
                        }
                        //    putText(frame,s1,Point2f(25,740), FONT_HERSHEY_PLAIN, 2,  Scalar(255,255,255));
                        // The following is what will be written on the image (in the tag)
                        stringstream ss;
                        ss << det->id;
                        // This is the text which is put on the image
                        String text = ss.str();

                        double fontscale = 1.0;
                        int baseline;
                        Size textsize = getTextSize(text, fontface, fontscale, 2,
                                                    &baseline);
                        putText(frame, text, Point(det->c[0]-textsize.width/2,
                                det->c[1]+textsize.height/2),
                                fontface, fontscale, Scalar(0xff, 0x99, 0), 2);
                        cv::circle(frame, cv::Point(cog[0], cog[1]), 20, CV_RGB(255,0,0));
                        cv::circle(frame, cv::Point(0, 0), 20, CV_RGB(0,255,0));
                        cv::circle(frame, cv::Point(1040, 776), 20, CV_RGB(0,0,255));

                        //                double dist_point_=0.167;
                        double tag_size = tagsize;

                        // Try to get the position of the tag w.r.t. the camera

                        apriltag2::AprilTagDetectionArray tag_detection_array;
                        apriltag2::AprilTagDetection tag_detection1;
                        geometry_msgs::PoseStamped poseTag1;
                        geometry_msgs::PoseArray tag_pose_array;
                        //                tag_pose_array.header = cv_ptr->header;
                        //                        if(det->id == 2 || det->id ==3 || det->id ==4){
                        matd_t *M = homography_to_pose(det->H, -fx, fy, cx, cy);
                        double scale = tagsize / 2.0;
                        MATD_EL(M, 0, 3) *= scale;
                        MATD_EL(M, 1, 3) *= scale;
                        MATD_EL(M, 2, 3) *= scale;

                        // ROS PART
                        // Eigen::Matrix4d transform = detection.getRelativeTransform(tag_size, fx, fy, px, py);
                        // Eigen::Matrix3d rot = transform.block(0, 0, 3, 3);
                        //MatrixXd eigenX = Map<MatrixXd>( X, nRows, nCols );
                        //                    Eigen::Matrix4d transform = Eigen::Map<Eigen::Matrix4d>transform(M->data,M->nrows,M->ncols);
                        Eigen::Map<Eigen::Matrix4d>transform(M->data);
                        //                    Map<Matrix4d> transform(M->data);
                        //                    cout<<"transform: " << transform.data();
                        //                    Eigen::Matrix3d rot = transform.block(0, 0, 3, 3);
                        //                    Eigen::Quaternion<double> rot_quaternion = Eigen::Quaternion<double>(rot);
                        geometry_msgs::PoseStamped tag_pose;
                        tag_pose.pose.position.x = MATD_EL(M, 0, 3);
                        tag_pose.pose.position.y = MATD_EL(M, 1, 3);
                        tag_pose.pose.position.z = MATD_EL(M, 2, 3);
                        //                    tag_pose.pose.orientation.x = rot_quaternion.x();
                        //                    tag_pose.pose.orientation.y = rot_quaternion.y();
                        //                    tag_pose.pose.orientation.z = rot_quaternion.z();
                        //                    tag_pose.pose.orientation.w = rot_quaternion.w();
                        tag_pose.header = cv_ptr->header;

                        apriltag2::AprilTagDetection tag_detection;

                        tag_detection.pose = tag_pose;
                        //                    tag_detection.id = detection.id;
                        tag_detection.id = det->id;
                        tag_detection.size = tag_size;
                        tag_detection_array.detections.push_back(tag_detection);
                        tag_pose_array.poses.push_back(tag_pose.pose);

                        //                            tf::Stamped<tf::Transform> tag_transform;
                        //                            tf::poseStampedMsgToTF(tag_pose, tag_transform);

                        if (det->id == 1) 
                        {
                            tag_detection1.pose = tag_pose;
                            //                    tag_detection.id = detection.id;
                            tag_detection1.id = det->id;
                            tag_detection1.size = tag_size;

                            //                                poseTag1.pose = tag_pose;
                            //                                poseTag1.header = 131088;


                        }

                        //                    tf_pub_.sendTransform(tf::StampedTransform(tag_transform, tag_transform.stamp_, tag_transform.frame_id_, description.frame_name()));
                        //                    chatter_pub.publish(message);

                        //                        cout << "Detection " << i << ": [" << MATD_EL(M, 0, 3) << ", " << MATD_EL(M, 1, 3) << ", " << MATD_EL(M, 2, 3) << "]" << endl;

                        detections_pub_.publish(tag_detection_array);
                        detections_pub_1.publish(tag_detection1);
                        poseStamped_pub_1.publish(poseTag1);
                    }
                    zarray_destroy(detections);
                    cv::destroyWindow("view");
                    imshow("Tag Detections", frame);
                    if (waitKey(30) >= 0)
                        break;
                }
                ros::spinOnce();
                loop_rate.sleep();
            } // END WHILE!


            ///////////////////////////////
            // cv::imshow("image", image);
            //        ros::spinOnce();
            //        key = cv::waitKey(30);
            //        ros::spinOnce();
            //        loop_rate.sleep();
        }


        //	Destruction of the apriltag_detector
        apriltag_detector_destroy(td);
        if (!strcmp(famname, "tag36h11"))
            tag36h11_destroy(tf);
        else if (!strcmp(famname, "tag36h10"))
            tag36h10_destroy(tf);
        else if (!strcmp(famname, "tag36artoolkit"))
            tag36artoolkit_destroy(tf);
        else if (!strcmp(famname, "tag25h9"))
            tag25h9_destroy(tf);
        else if (!strcmp(famname, "tag25h7"))
            tag25h7_destroy(tf);
        getopt_destroy(getopt);

    }


}
