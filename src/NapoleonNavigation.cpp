#include <ros/ros.h>

#include <iostream>
#include <Visualization/Visualization.h>
#include <Definitions/Polygon.h>
#include <Model/HolonomicModel.h>
#include <Model/BicycleModel.h>
#include <Obstacles/Obstacles.h>
#include <Tube/Tube.h>
#include <Tube/Tubes.h>
#include <cmath>
#include <vector>
#include <Visualization/VisualizationOpenCV.h>
#include <Visualization/VisualizationRviz.h>
#include <Communication/Communication.h>

typedef Vector2D Vec;

#define wall(p1, p2) Obstacle(Polygon({p1, p2}, Open), Pose2D(p1, 0), Static)
#define staticobstacle(poly, middle, pose) Obstacle(Polygon(poly, middle, Closed), pose, Static)
#define dynamicobstacle(poly, pose) Obstacle(Polygon(poly), pose, Dynamic)

double F_loop = 30;
double F_prediction = 10;

int main(int argc, char** argv) {
    cout << "Main loop started" << endl;

    //Polygon footprint({Vec(0,0), Vec(2,0), Vec(2, 0.2), Vec(2.3,0.2), Vec(2.3,0.8), Vec(2,0.8), Vec(2,1), Vec(0,1)}, Closed, true, Pose2D(1,0.5,0));
    Polygon footprint({Vec(0,0), Vec(0.65,0), Vec(0.65,0.6), Vec(0,0.6)}, Closed, true, Pose2D(0.325,0.3,0));
    HolonomicModel hmodel(Pose2D(0,0,M_PI_2), footprint, 1, 0.7, 0.25);

    Tubes tubes;
//    Tubes tubes(Tube(Vec(0,0), 1, Vec(-3,5), 1, 1));
//    tubes.addPoint(Vec(3,5), 1, 1);
//    tubes.addPoint(Vec(3,0.5), 1, 1);
//    tubes.addPoint(Vec(3,0), 1, 1);
    bool testRoute = false;

    ros::init(argc, argv, "route_navigation");

    ros::NodeHandle nroshndl("~");
    ros::Rate rate(F_loop);

    VisualizationRviz canvas(nroshndl);
    Communication comm(nroshndl);

    bool startNavigation = false;
    while(!startNavigation){
        if(ros::ok()) {
            ros::spinOnce();
            if(comm.initialized) {
                canvas.checkId();
                canvas.resetId();
                hmodel.update(1/F_loop, comm);
                hmodel.show(canvas, Color(0, 0, 0), Thin);
                hmodel.showCommunicationInput(canvas, Color(0, 0, 0), Thin, comm);
                comm.obstacles.show(canvas, Color(255,0,0), Thick);

                if(comm.newPlan()) {
                    tubes.convertRoute(comm.route, hmodel, canvas);
                    startNavigation = true;
                }
                if(testRoute){
                    startNavigation = true;
                }
                tubes.showSides(canvas);
            }
        }
        hmodel.update(1/F_loop, comm);
        rate.sleep();
    }

    FollowStatus realStatus = Status_Ok;
    FollowStatus predictionStatus = Status_Ok;
    FollowStatus prevRealStatus = Status_Error;
    FollowStatus prevPredictionStatus = Status_Error;

    while(nroshndl.ok() && ros::ok()){

        canvas.checkId();
        canvas.resetId();

        //tubes.visualizePlan(comm.route, canvas);
        tubes.visualizeRightWall(comm.route, canvas);

        canvas.arrow(Vec(0,0),Vec(1,0),Color(0,0,0),Thin);
        canvas.arrow(Vec(0,0),Vec(0,1),Color(0,0,0),Thin);

        if(startNavigation){
            HolonomicModel hmodelCopy = hmodel;
            predictionStatus = hmodelCopy.predict(10, 4, 0.3, 1/F_prediction, hmodel, tubes, canvas); //nScaling | predictionTime | minDistance
            hmodel.copySettings(hmodelCopy);

            //status = Status_Ok;

            if(predictionStatus == Status_Ok || predictionStatus == Status_Done) {
                realStatus = hmodel.follow(tubes, canvas, true);
                //tubes.avoidObstacles(hmodel.currentTubeIndex, hmodel.currentTubeIndex, comm.obstacles, hmodel, DrivingSide_Right, canvas);
                if(realStatus != Status_Ok) {hmodel.brake();}
            }else{
                hmodel.brake();
            }

            if(realStatus != prevRealStatus) {
                switch (realStatus) {
                    case Status_Ok: {cout << "Status Ok" << endl;break;}
                    case Status_ToClose: {cout << "Status To Close" << endl;break;}
                    case Status_Stuck: {cout << "Status Stuck" << endl;break;}
                    case Status_Error: {cout << "Status Error" << endl;break;}
                    case Status_Collision: {cout << "Status Collision" << endl;break;}
                    case Status_Done: {cout << "Status Done" << endl;break;}
                }
                prevRealStatus = realStatus;
            }
            if(prevPredictionStatus != predictionStatus){
                switch (predictionStatus){
                    case Status_Ok: {cout << "prediction Status Ok" << endl;break;}
                    case Status_ToClose: {cout << "Prediction status To Close" << endl; break;}
                    case Status_Stuck: {cout << "Prediction status Stuck" << endl; break;}
                    case Status_Error: {cout << "Prediction status Error" << endl; break;}
                    case Status_Collision: {cout << "Prediction status Collision" << endl; break;}
                    case Status_Done: {cout << "Prediction status Done" << endl;break;}
                }
                prevPredictionStatus = predictionStatus;
            }
        }

        if(realStatus == Status_Done){
            hmodel.brake();
            startNavigation = false;
        }
        if(!startNavigation && comm.newPlan()) {
            tubes.convertRoute(comm.route, hmodel, canvas);
            startNavigation = true;
        }

        tubes.showSides(canvas);
        hmodel.show(canvas, Color(0,0,0), Thin);
        hmodel.showCommunicationInput(canvas, Color(0,255,50), Thin, comm);
        comm.obstacles.show(canvas, Color(255,0,0), Thick);

        double ct = rate.cycleTime().toSec();
        double ect = rate.expectedCycleTime().toSec();
        double cycleTime = ct > ect ? ct : ect;

        hmodel.update(cycleTime, comm);

        ros::spinOnce();
        rate.sleep();
        
    }
    ros::shutdown();
    return 0;
}

// Measure time >>>>>>>>>>>>>>>>>>
//long long m1 = cv::getTickCount();
//Code
//long long m2 = cv::getTickCount();
//cout << (1000*(m2-m1))/cv::getTickFrequency() << " ms" << endl;
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
