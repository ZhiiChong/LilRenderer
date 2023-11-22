#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), faces_(), uv_verts_(), vn_verts_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) {
                iss >> v[i];
            }
            verts_.push_back(v);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<int> f;
            int idx, idxvt, idxvn;
            iss >> trash;
            while (iss >> idx >> trash >> idxvt >> trash >> idxvn) {
                idx--; // in wavefront obj all indices start at 1, not zero
                idxvt--;
                idxvn--;
                f.push_back(idx);
                f.push_back(idxvt);
                f.push_back(idxvn);
            }
            faces_.push_back(f);
        } else if (!line.compare(0, 4, "vt  ")) {
            // 2 char
            iss >> trash;
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) {
                iss >> v[i];
                // std::cerr << "vt: " << v[i] << std::endl;
            }
            uv_verts_.push_back(v);
        } else if (!line.compare(0, 4, "vn  ")) {
            // 2 char
            iss >> trash;
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) {
                iss >> v[i];
                // std::cerr << "vt: " << v[i] << std::endl;
            }
            vn_verts_.push_back(v);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << " uv# " << uv_verts_.size() << " vn# " << vn_verts_.size() << std::endl;
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

int Model::nuv_verts() {
    return (int)uv_verts_.size();
}

int Model::nvn_verts() {
    return (int)vn_verts_.size();
}

std::vector<int> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::uv_vert(int i) {
    return uv_verts_[i];
}

Vec3f Model::vn_vert(int i) {
    return vn_verts_[i];
}