/***************************************************************************
 *   Copyright (C) 2010 by Markus Bader               *
 *   markus.bader@tuwien.ac.at                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/** @file

@brief ROS checkerboard pose detector.
The checkerboard_detection package can be used to determinate
the position and orientation of a checkerboard of known size within a
image with calibration information. The position of the board/camera will be published as [[tf|tf frame]].
In the background the node uses the OpenCV funktion cv::findChessboardCorners, cvFindCornerSubPix and cv::solvePnP.

@par Advertises

 - @b visualization_marker topic (visualization_msgs/Marker) Visualization marker for [[rviz]]

 - @b /tf topic (tf/tfMessage), the pose of a borad or the camera relative to the base_frame (the parameters used)

*/


#include <estimate_pose.h>
#include <estimate_pose_default_values.h>
#include <opencv2/nonfree/nonfree.hpp>


int main ( int argc, char **argv )
{
    ros::init ( argc, argv, "camera_pose" );
    cvStartWindowThread();
    cv::initModule_nonfree();
    ros::NodeHandle nh;
    ros::NodeHandle private_nh ( "~" );

    cv::Size_<int>  checkerboard;
    cv::Size_<double> checkerboard_box;
    double loopFrq;


    std::string base_frame(BASE_FRAME);
    private_nh.getParam ( "base_frame", base_frame );
    ROS_INFO("\tBase frame: %s", base_frame.c_str());

    std::string frame_id ( FRAME_ID );
    private_nh.getParam ( "frame_id", frame_id );
    ROS_INFO("\tFrame id: %s", frame_id.c_str());

    std::string marker_ns ( MARKER_NS );
    private_nh.getParam ( "marker_ns", marker_ns );
    ROS_INFO("\tMarker Namespace: %s", marker_ns.c_str());

    int skip_frames = SKIP_FRAMES;
    private_nh.getParam ( "skip_frames", skip_frames);
    ROS_INFO("\tSkip frames: %i", skip_frames);

    std::string service_name(SERVICE_NAME);
    private_nh.getParam ( "service_name", service_name );
    ROS_INFO("\tService name: %s", service_name.c_str());

    std::string pose_file(POSE_FILE);
    private_nh.getParam ( "pose_file", pose_file );
    ROS_INFO("\tPose file: %s", pose_file.c_str());

    bool read_pose_file = READ_POSE_FILE;
    private_nh.getParam("read_pose_file", read_pose_file);
    ROS_INFO("\tRead pose file: %d", read_pose_file);

    bool draw_debug_image = DRAW_DEBUG_IMAGES;
    private_nh.getParam("draw_debug_image", draw_debug_image);
    ROS_INFO("\tDraw debug image: %d", draw_debug_image);

    bool publish_rviz_marker = PUBLISH_RVIZ_MARKER;
    private_nh.getParam("publish_rviz_marker", publish_rviz_marker);
    ROS_INFO("\tPublish rviz marker: %d", publish_rviz_marker);

    bool publish_tf_link = PUBLISH_TF_LINK;
    private_nh.getParam("publish_tf_link", publish_tf_link);
    ROS_INFO("\tPublish tf link: %d", publish_tf_link);

    bool publish_last_success = PUBLISH_LAST_SUCCESS;
    private_nh.getParam("publish_last_success", publish_last_success);
    ROS_INFO("\tPublish last success: %d", publish_last_success);

    bool publish_camera_pose = PUBLISH_CAMERA_POSE;
    private_nh.getParam("publish_camera_pose", publish_camera_pose);
    ROS_INFO("\tPublish camera pose: %d", publish_camera_pose);

    bool use_sub_pixel = USE_SUB_PIXEL;
    private_nh.getParam("use_sub_pixel", use_sub_pixel);
    ROS_INFO("\tUse sub pixel: %d", use_sub_pixel);

    checkerboard.width = CHECKERBOARD_WIDTH;
    private_nh.getParam ( "checkerboard_width", checkerboard.width);
    ROS_INFO("\tCheckerboard width: %i", checkerboard.width);

    checkerboard.height = CHECKERBOARD_HEIGHT;
    private_nh.getParam ( "checkerboard_height", checkerboard.height);
    ROS_INFO("\tCheckerboard height: %i", checkerboard.height);

    checkerboard_box.width = CHECKERBOARD_BOXES_WIDTH;
    private_nh.getParam ( "checkerboard_box_width", checkerboard_box.width);
    ROS_INFO("\tCheckerboard height: %f", checkerboard_box.width);

    checkerboard_box.height = CHECKERBOARD_BOXES_HEIGHT;
    private_nh.getParam ( "checkerboard_box_height", checkerboard_box.height);
    ROS_INFO("\tCheckerboard height: %f", checkerboard_box.height);


    private_nh.param ( "loopFrq", loopFrq, DEFAULT_LOOP_RATE );


    std::string image_topic = nh.resolveName ( SRC_IMAGE_TOPIC );
    PoseDetector cam_pose ( nh, image_topic, publish_tf_link,  publish_rviz_marker, read_pose_file, draw_debug_image );
    cam_pose.initCheckerboard ( checkerboard, checkerboard_box, use_sub_pixel, publish_last_success );
    cam_pose.initLinks ( base_frame, frame_id );
    cam_pose.initMarkerNS ( marker_ns );
    cam_pose.initPoseFile ( pose_file );
    cam_pose.initSkipCount ( skip_frames );


    if (publish_camera_pose) {
        cam_pose.computeCameraPose ( );
    }
    if (skip_frames > 0) {
        cam_pose.subscribe();
    } else {
        ROS_INFO("\tSkip frame count is negative, images are ONLY processed on request");
    }

    cam_pose.initService(service_name);

    ros::Rate rate ( loopFrq );
    while ( ros::ok() )
    {
        ros::spin();
    }
}


PoseDetector::PoseDetector ( ros::NodeHandle nh, std::string image_topic, bool publish_tf_link,  bool publish_rviz_marker, bool read_pose_file, bool drawDebugImage)
        : mNoteHandle ( "~" )
        , mImageTransport ( nh )
        , mImageTopic ( image_topic )
        , mWindowName ( std::string ( "Debug_" ) + image_topic )
        , mPublishTFLink ( publish_tf_link )
        , mPublishRVizMarker ( publish_rviz_marker )
        , mReadPoseFromFile(read_pose_file)
	, mValidPose(false)
        , mDrawDebugImage ( drawDebugImage )
        , mPose2Compute(BOARD)
        , mPublishLastSuccess(false)
        , mSkipCount(1)
        , mFrameCount(0)
        , mSuccessfulDetection(false)
{
    if (mDrawDebugImage && !mReadPoseFromFile) {
        cv::namedWindow(mWindowName);
    }
    if (publish_rviz_marker) {
        mMarker_pub = nh.advertise<visualization_msgs::Marker>("visualization_marker", 1);
    }
}

PoseDetector::~PoseDetector() {
}

const std::string PoseDetector::windowName()
{
    return mWindowName;
}

void PoseDetector::subscribe()
{
    if (mReadPoseFromFile) {
        mCameraSubscriber = mImageTransport.subscribeCamera ( mImageTopic, 1, &PoseDetector::imageDummyCallback, this );
	mPose.read(mPoseFile);
    } else {
        mCameraSubscriber = mImageTransport.subscribeCamera ( mImageTopic, 1, &PoseDetector::imageCallback, this );
    }

}
void PoseDetector::initCheckerboard ( cv::Size_<int> checkerboard, cv::Size_<double> checkerboardbox, bool use_sub_pixel, bool publish_last_success )
{
    mCheckerboard = checkerboard;
    mCheckerboardBox = checkerboardbox;
    init ( mCheckerboard, mCheckerboardBox, use_sub_pixel );
    mPublishLastSuccess = publish_last_success;
}
void PoseDetector::initLinks ( std::string base_frame, std::string frame_id )
{
    mBase_frame = base_frame;
    mFrame_id = frame_id;
}
void PoseDetector::initMarkerNS ( std::string markerNS )
{
    mMarkerNS = markerNS;
}
void PoseDetector::initPoseFile ( std::string posefile )
{
    mPoseFile = posefile;
}
void PoseDetector::initSkipCount ( int skip_frames )
{
    mSkipCount = skip_frames;
}

bool PoseDetector::compute_pose (pose_msgs::GetPose::Request &req, pose_msgs::GetPose::Response &resp)
{
    std::string topic = mCameraSubscriber.getInfoTopic ();
    if (topic.empty()) {
        subscribe();
        mSuccessfulDetection = false;
        mSkipCount = 1;
        while ( mSuccessfulDetection == false) {
            ros::spinOnce();
        };
        mCameraSubscriber.shutdown ();
        mCameraSubscriber = image_transport::CameraSubscriber ();
    }
    resp.pose.position.x = mPose.x();
    resp.pose.position.y = mPose.y();
    resp.pose.position.z = mPose.z();
    cv::Mat_<double> quat = mPose.quaterion();
    resp.pose.orientation.x = quat(0);
    resp.pose.orientation.y = quat(1);
    resp.pose.orientation.z = quat(2);
    resp.pose.orientation.w = quat(3);
    return true;
}

void PoseDetector::initService ( std::string service_name )
{
    if (service_name.empty()) return;
    ros::NodeHandle nh_toplevel_;
    mService = mNoteHandle.advertiseService(service_name, &PoseDetector::compute_pose, this);
}
void PoseDetector::computeCameraPose() {
    mPose2Compute = CAMERA;
}

void PoseDetector::publishMarker()
{
    if (!mPublishRVizMarker) return;
    if (!mValidPose) return;
    visualization_msgs::Marker marker;
    marker.header.frame_id = mBase_frame;
    marker.header.stamp = mTime;
    marker.ns = mMarkerNS;
    marker.id = 0;
    marker.type = visualization_msgs::Marker::CUBE;
    marker.action = visualization_msgs::Marker::ADD;
    marker.pose.position.x = mPose.x();
    marker.pose.position.y = mPose.y();
    marker.pose.position.z = mPose.z();
    cv::Mat_<double> quat = mPose.quaterion();
    marker.pose.orientation.x = quat(0);
    marker.pose.orientation.y = quat(1);
    marker.pose.orientation.z = quat(2);
    marker.pose.orientation.w = quat(3);
    marker.scale.x = mCheckerboardBox.width*2;
    marker.scale.y = mCheckerboardBox.height*2;
    marker.scale.z = ( mCheckerboardBox.height + mCheckerboardBox.width ) / 20;
    marker.color.r = 0.0f;
    marker.color.g = 1.0f;
    marker.color.b = 0.0f;
    marker.color.a = 1.0;
    marker.lifetime = ros::Duration(5);
    mMarker_pub.publish ( marker );

}
void PoseDetector::publishTf()
{
    if (!mPublishTFLink) return;
    if (!mValidPose) return;
    cv::Mat_<double> quat = mPose.quaterion();
    tf::Transform transform(tf::Quaternion(quat(0), quat(1), quat(2), quat(3)),tf::Vector3 ( mPose.x(), mPose.y(), mPose.z()));
    mTF_broadcaster.sendTransform ( tf::StampedTransform ( transform, mTime, mBase_frame, mFrame_id ) );
}
void PoseDetector::imageDummyCallback ( const sensor_msgs::ImageConstPtr& image_msg,
                                        const sensor_msgs::CameraInfoConstPtr& info_msg )
{
    mTime = image_msg->header.stamp;
    publishTf();
    publishMarker();
    mSuccessfulDetection = true;
}
void PoseDetector::imageCallback ( const sensor_msgs::ImageConstPtr& image_msg,
                                   const sensor_msgs::CameraInfoConstPtr& info_msg )
{

    mFrameCount++;
    if (mSkipCount < 1) return;
    if ((mFrameCount % mSkipCount) > 0) return;

    cv_bridge::CvImagePtr cv_ptr;
    image_geometry::PinholeCameraModel cam_model;
    try
    {

        cv::Mat imgGray;
        // Bayer case copied from image_proc
        if (image_msg->encoding.find("bayer") != std::string::npos) {
            // Construct cv::Mat pointing to raw_image data
            const cv::Mat imgBayer(image_msg->height, image_msg->width, CV_8UC1, const_cast<uint8_t*>(&image_msg->data[0]), image_msg->step);
            cv::Mat imgBGR;
            // Convert to color BGR
            /// @todo Faster to convert directly to mono when color is not requested, but OpenCV doesn't support
            int code = 0;
            if (image_msg->encoding == sensor_msgs::image_encodings::BAYER_RGGB8)
                code = CV_BayerBG2BGR;
            else if (image_msg->encoding == sensor_msgs::image_encodings::BAYER_BGGR8)
                code = CV_BayerRG2BGR;
            else if (image_msg->encoding == sensor_msgs::image_encodings::BAYER_GBRG8)
                code = CV_BayerGR2BGR;
            else if (image_msg->encoding == sensor_msgs::image_encodings::BAYER_GRBG8)
                code = CV_BayerGB2BGR;
            else {
                ROS_ERROR("[image_proc] Unsupported encoding '%s'", image_msg->encoding.c_str());
                return;
            }
            cv::cvtColor(imgBayer, imgBGR, code);
            cv::cvtColor(imgBGR, imgGray, CV_BGR2GRAY);
        } else {
	  //Commented out because I'm not getting a grey scale image right now so I don't care --JW
	  //cv_bridge::CvImagePtr cv_ptr;
	  //cv_ptr = cv_bridge::toCvCopy( image_msg, sensor_msgs::image_encodings::MONO8 );
	  //imgGray=*cv_ptr;
        }
        cam_model.fromCameraInfo ( info_msg );
        mTime = image_msg->header.stamp;
	V4R::PoseD pose;
        if (find ( imgGray, cam_model.projectionMatrix(), cam_model.distortionCoeffs (), pose.rvec(), pose.tvec()))
        {
            switch (mPose2Compute) {
            case BOARD:
	      mPose = pose;
                break;
            case CAMERA:
	      mFrame_id = info_msg->header.frame_id;
	      mPose = pose.inv();
                break;
            }
            if (!mPoseFile.empty()) {
	      mPose.write(mPoseFile, mBase_frame, mFrame_id);
            }
            mValidPose = true;
            publishTf();
            publishMarker();

            if ( mDrawDebugImage ) {
                drawBoard ( imgGray );
                drawSystem ( imgGray, cam_model.projectionMatrix(), cam_model.distortionCoeffs (), mPose.rvec(), mPose.tvec() );
            }
            mSuccessfulDetection = true;
        } else {
            if (mPublishLastSuccess) {
                publishTf();
                publishMarker();
            }
        }

        if ( mDrawDebugImage )
        {
            cv::imshow ( mWindowName, imgGray );
            cv::waitKey ( 100 );
        }
    }
    catch ( cv_bridge::Exception & ex )
    {
        ROS_ERROR ( "[draw_frames] Failed to convert image" );
        return;
    }
}
