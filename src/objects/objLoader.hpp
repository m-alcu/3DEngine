#pragma once


#include "solid.hpp"

class ObjLoader : public Solid {
    public:
        ObjLoader()
    {
    }

    public:
        void setup(const std::string& filename);

    private:
        void loadVertices(const std::string& filename);
        bool hasLoadedNormals = false;

    protected:   
        void loadVertices() override;
        void loadFaces() override;

    };

