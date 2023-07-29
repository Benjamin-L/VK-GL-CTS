#ifndef _GLCSHADERINDEXINGTESTS_HPP
#define _GLCSHADERINDEXINGTESTS_HPP
/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2016 Google Inc.
 * Copyright (c) 2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief Shader indexing (arrays, vector, matrices) tests.
 */ /*-------------------------------------------------------------------*/

#ifndef _GLCTESTCASE_HPP
#include "glcTestCase.hpp"
#endif
#ifndef _GLUSHADERUTIL_HPP
#include "gluShaderUtil.hpp"
#endif

namespace deqp
{

class ShaderIndexingTests : public TestCaseGroup
{
public:
    ShaderIndexingTests(Context &context, glu::GLSLVersion glslVersion);
    virtual ~ShaderIndexingTests(void);

    virtual void init(void);

private:
    ShaderIndexingTests(const ShaderIndexingTests &);            // not allowed!
    ShaderIndexingTests &operator=(const ShaderIndexingTests &); // not allowed!

    glu::GLSLVersion m_glslVersion;
};

} // namespace deqp

#endif // _GLCSHADERINDEXINGTESTS_HPP
