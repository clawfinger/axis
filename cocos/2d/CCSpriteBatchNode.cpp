/****************************************************************************
Copyright (c) 2009-2010 Ricardo Quesada
Copyright (c) 2009      Matt Oswald
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2011      Zynga Inc.
Copyright (c) 2013-2016 Chukong Technologies Inc.
Copyright (c) 2017-2018 Xiamen Yaji Software Co., Ltd.

http://www.cocos2d-x.org

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
#include "2d/CCSpriteBatchNode.h"
#include <stddef.h> // offsetof
#include "base/ccTypes.h"
#include "2d/CCSprite.h"
#include "base/CCDirector.h"
#include "base/CCProfiling.h"
#include "base/ccUTF8.h"
#include "renderer/CCTextureCache.h"
#include "renderer/CCRenderer.h"
#include "renderer/CCQuadCommand.h"
#include "renderer/ccShaders.h"
#include "renderer/backend/ProgramState.h"
#include "renderer/backend/Device.h"

NS_CC_BEGIN

/*
* creation with Texture2D
*/

SpriteBatchNode* SpriteBatchNode::createWithTexture(Texture2D* tex, ssize_t capacity/* = DEFAULT_CAPACITY*/)
{
    SpriteBatchNode *batchNode = new (std::nothrow) SpriteBatchNode();
    if(batchNode && batchNode->initWithTexture(tex, capacity))
    {
        batchNode->autorelease();
        return batchNode;
    }
    
    delete batchNode;
    return nullptr;
}

/*
* creation with File Image
*/

SpriteBatchNode* SpriteBatchNode::create(const std::string& fileImage, ssize_t capacity/* = DEFAULT_CAPACITY*/)
{
    SpriteBatchNode *batchNode = new (std::nothrow) SpriteBatchNode();
    if(batchNode && batchNode->initWithFile(fileImage, capacity))
    {
        batchNode->autorelease();
        return batchNode;
    }
    
    delete batchNode;
    return nullptr;
}

/*
* init with Texture2D
*/
bool SpriteBatchNode::initWithTexture(Texture2D *tex, ssize_t capacity/* = DEFAULT_CAPACITY*/)
{
    if(tex == nullptr)
    {
        return false;
    }
    
    CCASSERT(capacity>=0, "Capacity must be >= 0");
    
    _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
    if(!tex->hasPremultipliedAlpha())
    {
        _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
    }
    _textureAtlas = new (std::nothrow) TextureAtlas();

    if (capacity <= 0)
    {
        capacity = DEFAULT_CAPACITY;
    }
    
    _textureAtlas->initWithTexture(tex, capacity);

    setProgramStateWithRegistry(backend::ProgramType::POSITION_TEXTURE_COLOR, tex);
    updateProgramStateTexture(_textureAtlas->getTexture());

    updateBlendFunc();

    _children.reserve(capacity);

    _descendants.reserve(capacity);

    return true;
}

void SpriteBatchNode::setProgramState(backend::ProgramState* programState)
{
    auto& pipelineDescriptor = _quadCommand.getPipelineDescriptor();
    if (_programState != programState)
    {
        CC_SAFE_RELEASE(_programState);
        _programState = programState;
        CC_SAFE_RETAIN(programState);
    }
    pipelineDescriptor.programState = _programState;
    
    CC_SAFE_RELEASE(program);
    
    setVertexLayout();
    setUniformLocation();
}

void SpriteBatchNode::setUniformLocation()
{
    CCASSERT(_programState, "programState should not be nullptr");
    _mvpMatrixLocaiton = _programState->getUniformLocation("u_MVPMatrix");
    _textureLocation = _programState->getUniformLocation("u_texture");
}

void SpriteBatchNode::setVertexLayout()
{
    CCASSERT(_programState, "programState should not be nullptr");
    //set vertexLayout according to V3F_C4B_T2F structure
    auto vertexLayout = _programState->getVertexLayout();
    ///a_position
    vertexLayout->setAttribute(backend::ATTRIBUTE_NAME_POSITION,
                              _programState->getAttributeLocation(backend::Attribute::POSITION),
                              backend::VertexFormat::FLOAT3,
                              0,
                              false);
    ///a_texCoord
    vertexLayout->setAttribute(backend::ATTRIBUTE_NAME_TEXCOORD,
                              _programState->getAttributeLocation(backend::Attribute::TEXCOORD),
                              backend::VertexFormat::FLOAT2,
                              offsetof(V3F_C4B_T2F, texCoords),
                              false);
    
    ///a_color
    vertexLayout->setAttribute(backend::ATTRIBUTE_NAME_COLOR,
                              _programState->getAttributeLocation(backend::Attribute::COLOR),
                              backend::VertexFormat::UBYTE4,
                              offsetof(V3F_C4B_T2F, colors),
                              true);
    vertexLayout->setLayout(sizeof(V3F_C4B_T2F));
}

void SpriteBatchNode::setProgramState(backend::ProgramState *programState)
{
    CCASSERT(programState, "programState should not be nullptr");
    auto& pipelineDescriptor = _quadCommand.getPipelineDescriptor();
    if (_programState != programState)
    {
        CC_SAFE_RELEASE(_programState);
        _programState = programState;
        CC_SAFE_RETAIN(programState);
    }
    pipelineDescriptor.programState = _programState;
    
    setVertexLayout();
    setUniformLocation();
}

bool SpriteBatchNode::init()
{
    Texture2D * texture = new (std::nothrow) Texture2D();
    texture->autorelease();
    return this->initWithTexture(texture, 0);
}

/*
* init with FileImage
*/
bool SpriteBatchNode::initWithFile(const std::string& fileImage, ssize_t capacity/* = DEFAULT_CAPACITY*/)
{
    Texture2D *texture2D = Director::getInstance()->getTextureCache()->addImage(fileImage);
    return initWithTexture(texture2D, capacity);
}

SpriteBatchNode::SpriteBatchNode()
{
}

SpriteBatchNode::~SpriteBatchNode()
{
    CC_SAFE_RELEASE(_textureAtlas);
}

// override visit
// don't call visit on it's children
void SpriteBatchNode::visit(Renderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags)
{
    CC_PROFILER_START_CATEGORY(kProfilerCategoryBatchSprite, "CCSpriteBatchNode - visit");

    // CAREFUL:
    // This visit is almost identical to CocosNode#visit
    // with the exception that it doesn't call visit on it's children
    //
    // The alternative is to have a void Sprite#visit, but
    // although this is less maintainable, is faster
    //
    if (! _visible)
    {
        return;
    }

    sortAllChildren();

    uint32_t flags = processParentFlags(parentTransform, parentFlags);

    if (isVisitableByVisitingCamera())
    {
        // IMPORTANT:
        // To ease the migration to v3.0, we still support the Mat4 stack,
        // but it is deprecated and your code should not rely on it
        _director->pushMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
        _director->loadMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW, _modelViewTransform);
        
        draw(renderer, _modelViewTransform, flags);
        
        _director->popMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_MODELVIEW);
        // FIX ME: Why need to set _orderOfArrival to 0??
        // Please refer to https://github.com/cocos2d/cocos2d-x/pull/6920
        //    setOrderOfArrival(0);
        
        CC_PROFILER_STOP_CATEGORY(kProfilerCategoryBatchSprite, "CCSpriteBatchNode - visit");
    }
}

void SpriteBatchNode::addChild(Node *child, int zOrder, int tag)
{
    CCASSERT(child != nullptr, "child should not be null");
    CCASSERT(dynamic_cast<Sprite*>(child) != nullptr, "CCSpriteBatchNode only supports Sprites as children");
    Sprite *sprite = static_cast<Sprite*>(child);
    // check Sprite is using the same texture id
    CCASSERT(sprite->getTexture()->getBackendTexture() == _textureAtlas->getTexture()->getBackendTexture(), "CCSprite is not using the same texture id");

    Node::addChild(child, zOrder, tag);

    appendChild(sprite);
}

void SpriteBatchNode::addChild(Node * child, int zOrder, const std::string &name)
{
    CCASSERT(child != nullptr, "child should not be null");
    CCASSERT(dynamic_cast<Sprite*>(child) != nullptr, "CCSpriteBatchNode only supports Sprites as children");
    Sprite *sprite = static_cast<Sprite*>(child);
    // check Sprite is using the same texture id
    CCASSERT(sprite->getTexture() == _textureAtlas->getTexture(), "CCSprite is not using the same texture id");
    
    Node::addChild(child, zOrder, name);
    
    appendChild(sprite);
}

// override reorderChild
void SpriteBatchNode::reorderChild(Node *child, int zOrder)
{
    CCASSERT(child != nullptr, "the child should not be null");
    CCASSERT(_children.contains(child), "Child doesn't belong to Sprite");

    if (zOrder == child->getLocalZOrder())
    {
        return;
    }

    //set the z-order and sort later
    Node::reorderChild(child, zOrder);
}

// override remove child
void SpriteBatchNode::removeChild(Node *child, bool cleanup)
{
    Sprite *sprite = static_cast<Sprite*>(child);

    // explicit null handling
    if (sprite == nullptr)
    {
        return;
    }

    CCASSERT(_children.contains(sprite), "sprite batch node should contain the child");

    // cleanup before removing
    removeSpriteFromAtlas(sprite);

    Node::removeChild(sprite, cleanup);
}

void SpriteBatchNode::removeChildAtIndex(ssize_t index, bool doCleanup)
{
    CCASSERT(index>=0 && index < _children.size(), "Invalid index");
    removeChild(_children.at(index), doCleanup);
}

void SpriteBatchNode::removeAllChildrenWithCleanup(bool doCleanup)
{
    // Invalidate atlas index. issue #569
    // useSelfRender should be performed on all descendants. issue #1216
    for(const auto &sprite: _descendants) {
        sprite->setBatchNode(nullptr);
    }

    Node::removeAllChildrenWithCleanup(doCleanup);

    _descendants.clear();
    if (_textureAtlas) {_textureAtlas->removeAllQuads();}
}

//override sortAllChildren
void SpriteBatchNode::sortAllChildren()
{
    if (_reorderChildDirty)
    {
        sortNodes(_children);

        //sorted now check all children
        if (!_children.empty())
        {
            //first sort all children recursively based on zOrder
            for(const auto &child: _children) {
                child->sortAllChildren();
            }

            ssize_t index=0;

            //fast dispatch, give every child a new atlasIndex based on their relative zOrder (keep parent -> child relations intact)
            // and at the same time reorder descendants and the quads to the right index
            for(const auto &child: _children) {
                Sprite* sp = static_cast<Sprite*>(child);
                updateAtlasIndex(sp, &index);
            }
        }

        _reorderChildDirty=false;
    }
}

void SpriteBatchNode::updateAtlasIndex(Sprite* sprite, ssize_t* curIndex)
{
    auto& array = sprite->getChildren();
    auto count = array.size();
    
    ssize_t oldIndex = 0;

    if( count == 0 )
    {
        oldIndex = sprite->getAtlasIndex();
        sprite->setAtlasIndex(*curIndex);
        if (oldIndex != *curIndex){
            swap(oldIndex, *curIndex);
        }
        (*curIndex)++;
    }
    else
    {
        bool needNewIndex=true;

        if (array.at(0)->getLocalZOrder() >= 0)
        {
            //all children are in front of the parent
            oldIndex = sprite->getAtlasIndex();
            sprite->setAtlasIndex(*curIndex);
            if (oldIndex != *curIndex)
            {
                swap(oldIndex, *curIndex);
            }
            (*curIndex)++;

            needNewIndex = false;
        }

        for(const auto &child: array) {
            Sprite* sp = static_cast<Sprite*>(child);
            if (needNewIndex && sp->getLocalZOrder() >= 0)
            {
                oldIndex = sprite->getAtlasIndex();
                sprite->setAtlasIndex(*curIndex);
                if (oldIndex != *curIndex) {
                    this->swap(oldIndex, *curIndex);
                }
                (*curIndex)++;
                needNewIndex = false;
            }
            
            updateAtlasIndex(sp, curIndex);
        }

        if (needNewIndex)
        {//all children have a zOrder < 0)
            oldIndex = sprite->getAtlasIndex();
            sprite->setAtlasIndex(*curIndex);
            if (oldIndex != *curIndex) {
                swap(oldIndex, *curIndex);
            }
            (*curIndex)++;
        }
    }
}

void SpriteBatchNode::swap(ssize_t oldIndex, ssize_t newIndex)
{
    CCASSERT(oldIndex>=0 && oldIndex < (int)_descendants.size() && newIndex >=0 && newIndex < (int)_descendants.size(), "Invalid index");

    V3F_C4B_T2F_Quad* quads = _textureAtlas->getQuads();
    std::swap( quads[oldIndex], quads[newIndex] );

    //update the index of other swapped item

    auto oldIt = std::next( _descendants.begin(), oldIndex );
    auto newIt = std::next( _descendants.begin(), newIndex );

    (*newIt)->setAtlasIndex(oldIndex);
//    (*oldIt)->setAtlasIndex(newIndex);

    std::swap( *oldIt, *newIt );
}

void SpriteBatchNode::reorderBatch(bool reorder)
{
    _reorderChildDirty=reorder;
}

void SpriteBatchNode::draw(Renderer *renderer, const Mat4 &transform, uint32_t flags)
{
    // Optimization: Fast Dispatch
    if( _textureAtlas->getTotalQuads() == 0 )
    {
        return;
    }

    for (const auto &child : _children)
    {
        child->updateTransform();
    }
    
    const auto& matrixProjection = Director::getInstance()->getMatrix(MATRIX_STACK_TYPE::MATRIX_STACK_PROJECTION);
    auto programState = _quadCommand.getPipelineDescriptor().programState;
    programState->setUniform(_mvpMatrixLocaiton, matrixProjection.m, sizeof(matrixProjection.m));
    // programState->setTexture(_textureLocation, 0, _textureAtlas->getTexture()->getBackendTexture());

    _quadCommand.init(_globalZOrder, _textureAtlas->getTexture(), _blendFunc, _textureAtlas->getQuads(), _textureAtlas->getTotalQuads(), transform, flags);
    renderer->addCommand(&_quadCommand);
}

void SpriteBatchNode::increaseAtlasCapacity()
{
    // if we're going beyond the current TextureAtlas's capacity,
    // all the previously initialized sprites will need to redo their texture coords
    // this is likely computationally expensive
    ssize_t quantity = (_textureAtlas->getCapacity() + 1) * 4 / 3;

    CCLOG("cocos2d: SpriteBatchNode: resizing TextureAtlas capacity from [%d] to [%d].",
        static_cast<int>(_textureAtlas->getCapacity()),
        static_cast<int>(quantity));

    if (! _textureAtlas->resizeCapacity(quantity))
    {
        // serious problems
        CCLOGWARN("cocos2d: WARNING: Not enough memory to resize the atlas");
        CCASSERT(false, "Not enough memory to resize the atlas");
    }
}

void SpriteBatchNode::reserveCapacity(ssize_t newCapacity)
{
    if (newCapacity <= _textureAtlas->getCapacity())
        return;

    if (! _textureAtlas->resizeCapacity(newCapacity))
    {
        // serious problems
        CCLOGWARN("cocos2d: WARNING: Not enough memory to resize the atlas");
        CCASSERT(false, "Not enough memory to resize the atlas");
    }
}

ssize_t SpriteBatchNode::rebuildIndexInOrder(Sprite *parent, ssize_t index)
{
    CCASSERT(index>=0 && index < _children.size(), "Invalid index");

    auto& children = parent->getChildren();
    for(const auto &child: children) {
        Sprite* sp = static_cast<Sprite*>(child);
        if (sp && (sp->getLocalZOrder() < 0))
        {
            index = rebuildIndexInOrder(sp, index);
        }
    }

    // ignore self (batch node)
    if (parent != static_cast<Ref*>(this))
    {
        parent->setAtlasIndex(index);
        index++;
    }

    for(const auto &child: children) {
        Sprite* sp = static_cast<Sprite*>(child);
        if (sp && (sp->getLocalZOrder() >= 0))
        {
            index = rebuildIndexInOrder(sp, index);
        }
    }

    return index;
}

ssize_t SpriteBatchNode::highestAtlasIndexInChild(Sprite *sprite)
{
    auto& children = sprite->getChildren();

    if (children.empty())
    {
        return sprite->getAtlasIndex();
    }
    else
    {
        return highestAtlasIndexInChild( static_cast<Sprite*>(children.back()));
    }
}

ssize_t SpriteBatchNode::lowestAtlasIndexInChild(Sprite *sprite)
{
    auto& children = sprite->getChildren();

    if (children.size() == 0)
    {
        return sprite->getAtlasIndex();
    }
    else
    {
        return lowestAtlasIndexInChild(static_cast<Sprite*>(children.at(0)));
    }
}

ssize_t SpriteBatchNode::atlasIndexForChild(Sprite *sprite, int nZ)
{
    auto& siblings = sprite->getParent()->getChildren();
    auto childIndex = siblings.getIndex(sprite);

    // ignore parent Z if parent is spriteSheet
    bool ignoreParent = (SpriteBatchNode*)(sprite->getParent()) == this;
    Sprite *prev = nullptr;
    if (childIndex > 0 && childIndex != -1)
    {
        prev = static_cast<Sprite*>(siblings.at(childIndex - 1));
    }

    // first child of the sprite sheet
    if (ignoreParent)
    {
        if (childIndex == 0)
        {
            return 0;
        }

        return highestAtlasIndexInChild(prev) + 1;
    }

    // parent is a Sprite, so, it must be taken into account

    // first child of an Sprite ?
    if (childIndex == 0)
    {
        Sprite *p = static_cast<Sprite*>(sprite->getParent());

        // less than parent and brothers
        if (nZ < 0)
        {
            return p->getAtlasIndex();
        }
        else
        {
            return p->getAtlasIndex() + 1;
        }
    }
    else
    {
        // previous & sprite belong to the same branch
        if ((prev->getLocalZOrder() < 0 && nZ < 0) || (prev->getLocalZOrder() >= 0 && nZ >= 0))
        {
            return highestAtlasIndexInChild(prev) + 1;
        }

        // else (previous < 0 and sprite >= 0 )
        Sprite *p = static_cast<Sprite*>(sprite->getParent());
        return p->getAtlasIndex() + 1;
    }

    // Should not happen. Error calculating Z on SpriteSheet
    CCASSERT(0, "should not run here");
    return 0;
}

// addChild helper, faster than insertChild
void SpriteBatchNode::appendChild(Sprite* sprite)
{
    _reorderChildDirty=true;
    sprite->setBatchNode(this);
    sprite->setDirty(true);

    if(_textureAtlas->getTotalQuads() == _textureAtlas->getCapacity()) {
        increaseAtlasCapacity();
    }

    _descendants.push_back(sprite);
    int index = static_cast<int>(_descendants.size()-1);

    sprite->setAtlasIndex(index);

    V3F_C4B_T2F_Quad quad = sprite->getQuad();
    _textureAtlas->insertQuad(&quad, index);

    // add children recursively
    auto& children = sprite->getChildren();
    for(const auto &child: children) {
#if CC_SPRITE_DEBUG_DRAW
        // when using CC_SPRITE_DEBUG_DRAW, a DrawNode is appended to sprites. remove it since only Sprites can be used
        // as children in SpriteBatchNode
        // Github issue #14730
        if (dynamic_cast<DrawNode*>(child)) {
            // to avoid calling Sprite::removeChild()
            sprite->Node::removeChild(child, true);
        }
        else
#else
        CCASSERT(dynamic_cast<Sprite*>(child) != nullptr, "You can only add Sprites (or subclass of Sprite) to SpriteBatchNode");
#endif
        appendChild(static_cast<Sprite*>(child));
    }
}

void SpriteBatchNode::removeSpriteFromAtlas(Sprite *sprite)
{
    // remove from TextureAtlas
    _textureAtlas->removeQuadAtIndex(sprite->getAtlasIndex());

    // Cleanup sprite. It might be reused (issue #569)
    sprite->setBatchNode(nullptr);

    auto it = std::find(_descendants.begin(), _descendants.end(), sprite );
    if( it != _descendants.end() )
    {
        auto next = std::next(it);

        Sprite *spr = nullptr;
        for(auto nextEnd = _descendants.end(); next != nextEnd; ++next) {
            spr = *next;
            spr->setAtlasIndex( spr->getAtlasIndex() - 1 );
        }

        _descendants.erase(it);
    }

    // remove children recursively
    auto& children = sprite->getChildren();
    for(const auto &obj: children) {
        Sprite* child = static_cast<Sprite*>(obj);
        if (child)
        {
            removeSpriteFromAtlas(child);
        }
    }
}

void SpriteBatchNode::updateBlendFunc()
{
    if (! _textureAtlas->getTexture()->hasPremultipliedAlpha())
    {
        _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
        setOpacityModifyRGB(false);
    }
    else
    {
        _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
        setOpacityModifyRGB(true);
    }
}

// CocosNodeTexture protocol
void SpriteBatchNode::setBlendFunc(const BlendFunc &blendFunc)
{
    _blendFunc = blendFunc;
}

const BlendFunc& SpriteBatchNode::getBlendFunc() const
{
    return _blendFunc;
}

Texture2D* SpriteBatchNode::getTexture() const
{
    return _textureAtlas->getTexture();
}

void SpriteBatchNode::setTexture(Texture2D *texture)
{
    _textureAtlas->setTexture(texture);

    updateProgramStateTexture(texture);
    updateBlendFunc();
}


// SpriteSheet Extension
//implementation SpriteSheet (TMXTiledMapExtension)

void SpriteBatchNode::insertQuadFromSprite(Sprite *sprite, ssize_t index)
{
    CCASSERT( sprite != nullptr, "Argument must be non-nullptr");
    CCASSERT( dynamic_cast<Sprite*>(sprite), "CCSpriteBatchNode only supports Sprites as children");

    // make needed room
    while(index >= _textureAtlas->getCapacity() || _textureAtlas->getCapacity() == _textureAtlas->getTotalQuads())
    {
        this->increaseAtlasCapacity();
    }
    //
    // update the quad directly. Don't add the sprite to the scene graph
    //
    sprite->setBatchNode(this);
    sprite->setAtlasIndex(index);

    V3F_C4B_T2F_Quad quad = sprite->getQuad();
    _textureAtlas->insertQuad(&quad, index);

    // FIXME:: updateTransform will update the textureAtlas too, using updateQuad.
    // FIXME:: so, it should be AFTER the insertQuad
    sprite->setDirty(true);
    sprite->updateTransform();
}

void SpriteBatchNode::updateQuadFromSprite(Sprite *sprite, ssize_t index)
{
    CCASSERT(sprite != nullptr, "Argument must be non-nil");
    CCASSERT(dynamic_cast<Sprite*>(sprite) != nullptr, "CCSpriteBatchNode only supports Sprites as children");
    
    // make needed room
    while (index >= _textureAtlas->getCapacity() || _textureAtlas->getCapacity() == _textureAtlas->getTotalQuads())
    {
        this->increaseAtlasCapacity();
    }
    
    //
    // update the quad directly. Don't add the sprite to the scene graph
    //
    sprite->setBatchNode(this);
    sprite->setAtlasIndex(index);
    
    sprite->setDirty(true);
    
    // UpdateTransform updates the textureAtlas quad
    sprite->updateTransform();
}

SpriteBatchNode * SpriteBatchNode::addSpriteWithoutQuad(Sprite*child, int z, int aTag)
{
    CCASSERT( child != nullptr, "Argument must be non-nullptr");
    CCASSERT( dynamic_cast<Sprite*>(child), "CCSpriteBatchNode only supports Sprites as children");

    // quad index is Z
    child->setAtlasIndex(z);

    // FIXME:: optimize with a binary search
    auto it = _descendants.begin();
    for (auto itEnd = _descendants.end(); it != itEnd; ++it)
    {
        if((*it)->getAtlasIndex() >= z)
            break;
    }

    _descendants.insert(it, child);

    // IMPORTANT: Call super, and not self. Avoid adding it to the texture atlas array
    Node::addChild(child, z, aTag);

    //#issue 1262 don't use lazy sorting, tiles are added as quads not as sprites, so sprites need to be added in order
    reorderBatch(false);

    return this;
}

std::string SpriteBatchNode::getDescription() const
{
    return StringUtils::format("<SpriteBatchNode | tag = %d>", _tag);
}

NS_CC_END
