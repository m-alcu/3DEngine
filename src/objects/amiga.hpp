#pragma once


#include "solid.hpp"

class Amiga : public Solid {
    public:
        // Constructor that takes the number of vertices and faces as parameters. https://stackoverflow.com/questions/16119762/destructor-of-an-object-causes-crashing
        Amiga()
    {
    }

    public:
       void setup(int lat, int lon);

    private:
        void loadVertices( int lat, int lon );
        void loadFaces(int lat, int lon);
        bool isRedTile(float u, float v, int rows = 8, int cols = 16);
        void calculateVertexNormals();

    protected:   
        void loadVertices() override;
        void loadFaces() override;
    };


