//
// Created by bob on 24-10-19.
//

#include "Obstacles.h"

void Obstacles::show(Visualization &canvas, Color c, int drawStyle) {
    canvas.idName = "Obstacles";
    for(auto &obstacle : obstacles){
        obstacle.show(canvas, c, drawStyle);
    }
}