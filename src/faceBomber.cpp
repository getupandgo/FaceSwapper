#include "faceBomber.h"

FaceBomber::FaceBomber() : faceDetector(dlib::get_frontal_face_detector()) {
    dlib::deserialize("/home/getupandgo/projects/FaceBomber/shape_predictor_model/shape_predictor_68_face_landmarks.dat") >> shapePredictor;
    getAllFacesForBombing();
}

void FaceBomber::doFancyFaceBomb(const std::string imagePath, const std::vector<std::string> newFaces) {
    dlib::array2d<dlib::rgb_pixel> targetImage;
    //loading and scaling up target image
    dlib::load_image(targetImage, imagePath);
    pyramid_up(targetImage);

    if (newFaces.size()) {
        std::cout << "No mask faces provided" << std::endl;
        throw "no faces found";
    } else if (newFaces.size() > 1) {
        std::cout << "using only first face for now: " << newFaces[0] << std::endl;
    }

    std::vector<cv::Point2i> maskFaceShapes;

    std::vector<dlib::full_object_detection> faceShapes = findFacesAndExtractPoses(targetImage);

    std::for_each(faceShapes.begin(), faceShapes.end(), [&](dlib::full_object_detection &faceShape) {
        std::vector<cv::Point2i> currentFaceShapes = dlibObjectDetectionToCvPointsArray(faceShape);

        replaceFace(currentFaceShapes, maskFaceShapes);

//        std::cout << dlibObjectDetectionToCvPointsArray(faceShape) << std::endl;
    });

    //debug display
    dlib::image_window win;

    win.clear_overlay();
    win.set_image(targetImage);
    win.add_overlay(dlib::render_face_detections(faceShapes));

    std::cout << "Pause..." << std::endl;
    std::cin.get();
}

void FaceBomber::doUglyFaceBomb(const std::string imagePath, const std::vector<std::string> newFaces) {

    if (!newFaces.size()) {
        std::cout << "No mask faces provided" << std::endl;
        throw "no mask provided";
    } else {
        std::cout << "using only first face for now: " << newFaces[0] << std::endl;
    }
    //loading image with new face
    dlib::array2d<dlib::rgb_pixel> newImage;
    dlib::load_image(newImage, imagePath);
    std::vector<dlib::rectangle> newFaceArray = faceDetector(newImage);
    dlib::rectangle newFace = newFaceArray[0];

    //loading and scaling up (for detecting small faces) target image
    dlib::array2d<dlib::rgb_pixel> targetImage;
    dlib::load_image(targetImage, imagePath);
    pyramid_up(targetImage);
    std::vector<dlib::rectangle> detectedFaces = faceDetector(targetImage);

    if (!newFaces.size()) {
        std::cout << "No faces found" << std::endl;
        throw "no faces found";
    } else {
        std::cout << "Number of faces detected: " << detectedFaces.size() << std::endl;
    }

    dlib::image_window win;

    std::for_each(detectedFaces.begin(), detectedFaces.end(), [&](dlib::rectangle &detectedFace) {
        std::cout << "scaling newface with width " << newFace.width() << " and height " << newFace.height() << " to " << detectedFace.width() << " " << detectedFace.height() << std::endl;
        dlib::rectangle scaledNewFace = resize_rect(newFace, detectedFace.width(), detectedFace.height());
        std::cout << "result: " << scaledNewFace.width() << " " << scaledNewFace.height() << std::endl;
        std::cout << "moving newface from " << scaledNewFace.tl_corner().x() << " " << scaledNewFace.tl_corner().y() << " to " << detectedFace.tl_corner().x() << " " << detectedFace.tl_corner().y() << std::endl;
        dlib::rectangle movedNewFace = move_rect(scaledNewFace, detectedFace.tl_corner());
        std::cout << "result: " << movedNewFace.tl_corner().x() << " " << movedNewFace.tl_corner().y() << std::endl;
        draw_rectangle(targetImage, movedNewFace, dlib::rgb_pixel(0,0,0), 0);

//        win.clear_overlay();
        win.set_image(targetImage);

        std::cout << "Pause..." << std::endl;
        std::cin.get();
    });

    //debug display
//    dlib::image_window win;

//    win.clear_overlay();
//    win.set_image(targetImage);
//
//    std::cout << "Pause..." << std::endl;
//    std::cin.get();
}

void FaceBomber::replaceFace(std::vector<cv::Point2i> currentFaceShapes, std::vector<cv::Point2i> maskFaceShapes) {
    std::vector<cv::Point2f> hull1;
    std::vector<cv::Point2f> hull2;
    std::vector<int> hullIndexes;

    // finding convex hull for target face.
    cv::convexHull(currentFaceShapes, hullIndexes, false, false); //returns array of indexes of points, that forms convex hull for target image

    // here we are pushing points that forms convex hull for target and new image (imposes convex hull from target image to new one)
    for(int i = 0; i < hullIndexes.size(); i++)
    {
        hull1.push_back(maskFaceShapes[hullIndexes[i]]);
        hull2.push_back(currentFaceShapes[hullIndexes[i]]);
    }

    // TODO:
}

void FaceBomber::addFaceForBombing(const std::string faceName, const std::string imagePath) {
    // read image
    dlib::array2d<dlib::rgb_pixel> faceToExtract;
    dlib::load_image(faceToExtract, imagePath);

    std::vector<dlib::full_object_detection> facePoseOfDetectedFace = findFacesAndExtractPoses(faceToExtract);

    dlib::full_object_detection facePose;

    if (!facePoseOfDetectedFace.size()) {
        std::cout << "Unable to extract face from image" << std::endl;
        throw "no faces found";
    } else if (facePoseOfDetectedFace.size() > 1) {
        std::cout << "too many faces detected; must be only one" << std::endl;
        throw "too many faces found";
    } else {
        std::cout << "face extracted" << std::endl;
        facePose = facePoseOfDetectedFace[0];
        facesForBombing.insert(std::pair<std::string, std::string>(faceName, imagePath));
    }

    std::vector<cv::Point2i> facePoints = dlibObjectDetectionToCvPointsArray(facePose);

    std::vector<int> hullContourIndexes;
    // creating convexHull around facePoints obtained using dlib
    cv::convexHull(facePoints, hullContourIndexes, false, false);
    for (int i=0; i<hullContourIndexes.size(); ++i) {
        std::cout << hullContourIndexes[i] << std::endl;
    }


//    cv::convexHull(facePoints);

    // save image to "faces" folder

}

void FaceBomber::getAllFacesForBombing() {

}

std::vector<dlib::full_object_detection> FaceBomber::findFacesAndExtractPoses(dlib::array2d<dlib::rgb_pixel> &targetImage) {
    // extract list of bounding boxes around all the faces
    std::vector<dlib::rectangle> detectedFaces = faceDetector(targetImage);
    std::cout << "Number of faces detected: " << detectedFaces.size() << std::endl;

    // Finding the pose of each detected face
    std::vector<dlib::full_object_detection> faceShapes;

    std::for_each(detectedFaces.begin(), detectedFaces.end(), [&](dlib::rectangle &face) {
        dlib::full_object_detection faceShape = shapePredictor(targetImage, face);
        faceShapes.push_back(faceShape);
    });

    return faceShapes;
}

cv::Point2i FaceBomber::fromDlibToCvPoint (const dlib::point &dlibPoint) {
    return cv::Point2i(dlibPoint.x(), dlibPoint.y());
};

std::vector<cv::Point2i> FaceBomber::dlibObjectDetectionToCvPointsArray (const dlib::full_object_detection &objDetection) {
    std::vector<cv::Point2i> cvPointsArray;
    size_t objDetectionPointsNumber = objDetection.num_parts();
    cvPointsArray.reserve(objDetectionPointsNumber);

    for(unsigned long i = 0; i < objDetectionPointsNumber; ++i) {
        dlib::point dlibPoint = objDetection.part(i);
        cvPointsArray.push_back(fromDlibToCvPoint(dlibPoint));
    }

    return cvPointsArray;
}
