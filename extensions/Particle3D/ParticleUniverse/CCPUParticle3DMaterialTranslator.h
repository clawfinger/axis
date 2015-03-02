/****************************************************************************
 Copyright (c) 2014 Chukong Technologies Inc.
 
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

#ifndef __CC_PU_PARTICLE_3D_MATERIAL_TRANSLATOR_H__
#define __CC_PU_PARTICLE_3D_MATERIAL_TRANSLATOR_H__

#include "Particle3D/ParticleUniverse/CCPUParticle3DScriptTranslator.h"
#include "Particle3D/ParticleUniverse/CCPUParticle3DScriptCompiler.h"

NS_CC_BEGIN
class PUParticle3DMaterial;
class PUParticle3DMaterialCache;
class  PUParticle3DMaterialTranslator : public PUScriptTranslator
{
public:
    PUParticle3DMaterialTranslator();
    virtual ~PUParticle3DMaterialTranslator();
    virtual void translate(PUScriptCompiler* compiler, PUAbstractNode *node);
    void setMaterialSystem(PUParticle3DMaterialCache *ms);

protected:
    PUParticle3DMaterial *_material;
    PUParticle3DMaterialCache *_ms;
};

class PUParticle3DMaterialTechniqueTranslator : public PUScriptTranslator
{
public:
    PUParticle3DMaterialTechniqueTranslator(){};
    virtual void translate(PUScriptCompiler* compiler, PUAbstractNode *node);
};

class PUParticle3DMaterialPassTranslator : public PUScriptTranslator
{
public:
    PUParticle3DMaterialPassTranslator(){};
    virtual void translate(PUScriptCompiler* compiler, PUAbstractNode *node);
};

class PUParticle3DMaterialTextureUnitTranslator : public PUScriptTranslator
{
public:
    PUParticle3DMaterialTextureUnitTranslator(){};
    virtual void translate(PUScriptCompiler* compiler, PUAbstractNode *node);
};

NS_CC_END


#endif /* defined(__cocos2d_libs__CCParticle3DTechniqueTranslator__) */
