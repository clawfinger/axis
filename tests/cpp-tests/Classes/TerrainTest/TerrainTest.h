/****************************************************************************
 Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

 https://axis-project.github.io/

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#ifndef TERRAIN_TESH_H

#include "../BaseTest.h"

#include "3d/CCMeshRenderer.h"
#include "3d/CCTerrain.h"
#include "2d/CCCamera.h"
#include "2d/CCAction.h"

DEFINE_TEST_SUITE(TerrainTests);

class TerrainTestDemo : public TestCase
{
protected:
    std::string _title;
};

class TerrainSimple : public TerrainTestDemo
{
public:
    CREATE_FUNC(TerrainSimple);
    TerrainSimple();
    virtual std::string title() const override;
    virtual std::string subtitle() const override;
    void onTouchesMoved(const std::vector<axis::Touch*>& touches, axis::Event* event);
    axis::Terrain* _terrain;

protected:
    axis::Camera* _camera;
};

#    define PLAYER_STATE_LEFT 0
#    define PLAYER_STATE_RIGHT 1
#    define PLAYER_STATE_IDLE 2
#    define PLAYER_STATE_FORWARD 3
#    define PLAYER_STATE_BACKWARD 4

class Player : public axis::MeshRenderer
{
public:
    static Player* create(const char* file, axis::Camera* cam, axis::Terrain* terrain);
    virtual bool isDone() const;
    virtual void update(float dt);

    void turnLeft();
    void turnRight();
    void forward();
    void backward();
    void idle();
    axis::Vec3 _targetPos;
    void updateState();
    float _headingAngle;
    axis::Vec3 _headingAxis;

private:
    axis::Terrain* _terrain;
    axis::Camera* _cam;
    int _playerState;
};

class TerrainWalkThru : public TerrainTestDemo
{
public:
    CREATE_FUNC(TerrainWalkThru);
    TerrainWalkThru();
    virtual std::string title() const override;
    virtual std::string subtitle() const override;
    void onTouchesBegan(const std::vector<axis::Touch*>& touches, axis::Event* event);
    void onTouchesEnd(const std::vector<axis::Touch*>& touches, axis::Event* event);

protected:
    axis::Camera* _camera;
    axis::Terrain* _terrain;
    Player* _player;
};

class TerrainWithLightMap : public TerrainTestDemo
{
public:
    CREATE_FUNC(TerrainWithLightMap);
    TerrainWithLightMap();
    virtual std::string title() const override;
    virtual std::string subtitle() const override;
    void onTouchesMoved(const std::vector<axis::Touch*>& touches, axis::Event* event);

protected:
    axis::Terrain* _terrain;
    axis::Camera* _camera;
};

#endif  // !TERRAIN_TESH_H
