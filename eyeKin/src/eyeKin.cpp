#include "eyeKin.h"

// Constructor and Destructor
personalRobotics::EyeKin::EyeKin() : tcpServer(PORT1, ADDRESS_FAMILY)
{
	// Configuration
	screenWidth = DEFAULT_SCREEN_WIDTH;
	screenHeight = DEFAULT_SCREEN_HEIGHT;

	// Start the server
	tcpServer.start();

	// Useful Images
	numCheckerPtsX = 0;
	numCheckerPtsY = 0;
	personalRobotics::createCheckerboard(checkerboard, screenWidth, screenHeight, numCheckerPtsX, numCheckerPtsY);

	//Flags
	tablePlaneFound.set(false);
	homographyFound.set(false);
	isCalibrating.set(true);

	// Counters
	epoch = 0;
}
personalRobotics::EyeKin::~EyeKin()
{
}

// Calibration methods
void personalRobotics::EyeKin::findTable()
{
	if(segmentor.findTablePlane())
		tablePlaneFound.set(true);
}
void personalRobotics::EyeKin::findHomography()
{
	/*std::vector<cv::Point2f> detectedCorners, checkerboardCorners;
	bool foundCorners = findChessboardCorners(*segmentor.getColorImagePtr(), cv::Size(numCheckerPtsX, numCheckerPtsY), detectedCorners, CV_CALIB_CB_ADAPTIVE_THRESH);
	bool foundProjectedCorners = findChessboardCorners(checkerboard, cv::Size(numCheckerPtsX, numCheckerPtsY), checkerboardCorners, CV_CALIB_CB_ADAPTIVE_THRESH);
	if (foundCorners)
	{
		cv::Mat invertedHomography = cv::findHomography(detectedCorners, checkerboardCorners, CV_RANSAC);
		cv::Mat homographyCorrection = (cv::Mat_<double>(3, 3) << 1, 0, 0, 0, -1, screenHeight, 0, 0, 1);
		homography = homographyCorrection*invertedHomography;
		homographyFound = true;
	}*/
	homography = (cv::Mat_<double>(3, 3) <<1.7456, 0.0337, -837.4711, -0.0143, -1.7469, 1411.6242, -0.0000, 0.0000, 1) * (cv::Mat_<double>(3, 3) << 1, 0, 0, 0, 1, 0, 0, 0, 1);
	homographyFound.set(true);
}
void personalRobotics::EyeKin::calibrate()
{
	while (isCalibrating.get())
	{
		if (!tablePlaneFound.get() && segmentor.isDepthAllocated.get())
			findTable();
		if (!homographyFound.get() && segmentor.isColorAllocated.get())
			findHomography();
		if (tablePlaneFound.get() && homographyFound.get())
		{
			isCalibrating.set(false);
			segmentor.setHomography(homography);

			// Map size of rgb and projector pixels
			std::vector<cv::Point2f> rgbKeyPoints;
			std::vector<cv::Point2f> projKeyPoints;
			rgbKeyPoints.push_back(cv::Point2f(0, 0));
			rgbKeyPoints.push_back(cv::Point2f(10, 0));
			rgbKeyPoints.push_back(cv::Point2f(0, 10));
			cv::perspectiveTransform(rgbKeyPoints, projKeyPoints, homography);
			float delX = 10.f / cv::norm(projKeyPoints[1] - projKeyPoints[0]);
			float delY = 10.f / cv::norm(projKeyPoints[2] - projKeyPoints[0]);
			colorPixelSize = *(segmentor.getRGBpixelSize());
			projPixelSize.x = colorPixelSize.x * delX;
			projPixelSize.y = colorPixelSize.y * delY;
			segmentor.startSegmentor();
		}
	}
}

// Routines
void personalRobotics::EyeKin::generateSerializableList(procamPRL::EntityList &serializableList)
{
	if (segmentor.newListGenerated.get())
	{
		// Lock the lists
		segmentor.lockList();

		// Set the non-serializable list to be old
		segmentor.newListGenerated.set(false);

		// Get access to entityList
		std::vector<personalRobotics::Entity> *listPtr = segmentor.getEntityList();

		// Copy information from the entity list
		serializableList.set_frameid(++epoch);
		serializableList.set_timestamp(0);
		
		for (std::vector<personalRobotics::Entity>::iterator entityPtr = listPtr->begin(); entityPtr != listPtr->end(); entityPtr++)
		{
			// Add an serializable entity to serializable entity list
			procamPRL::Entity* serializableEntityPtr = serializableList.add_entitylist();
			
			// Set pose
			personalRobotics::Pose2D pose;
			personalRobotics::Point2D position;
			position.set_x(entityPtr->pose2Dproj.position.x);
			position.set_y(entityPtr->pose2Dproj.position.y);
			pose.set_angle(entityPtr->pose2Dproj.angle);
			serializableEntityPtr->set_allocated_pose(&pose);

			// Set bounding size
			personalRobotics::Point2D boundingSize;
			boundingSize.set_x(entityPtr->boundingSize.width);
			boundingSize.set_y(entityPtr->boundingSize.height);
			serializableEntityPtr->set_allocated_boundingsize(&boundingSize);

			// Set pixel size
			personalRobotics::Point2D pixelSize;
			pixelSize.set_x(1);
			pixelSize.set_y(1);

			// Set contours
			for (std::vector<cv::Point>::iterator contourPointPtr = entityPtr->contour.begin(); contourPointPtr != entityPtr->contour.end(); contourPointPtr++)
			{
				personalRobotics::Point2D *serializableContourPtr = serializableEntityPtr->add_contours();
				serializableContourPtr->set_x(contourPointPtr->x);
				serializableContourPtr->set_y(contourPointPtr->y);
			}

			// Set Image
			procamPRL::Entity::Image image;
			image.set_height(entityPtr->patch.rows);
			image.set_width(entityPtr->patch.cols);
			image.set_data((void*)entityPtr->patch.data, entityPtr->patch.channels() * entityPtr->patch.rows * entityPtr->patch.cols);
			serializableEntityPtr->set_allocated_image(&image);
		}

		// Unlock the lists
		segmentor.unlockList();
	}
}

// Accessors
personalRobotics::TcpServer* personalRobotics::EyeKin::getServer()
{
	return &tcpServer;
}
personalRobotics::ObjectSegmentor* personalRobotics::EyeKin::getSegmentor()
{
	return &segmentor;
}