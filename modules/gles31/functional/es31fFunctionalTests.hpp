#ifndef _ES31FFUNCTIONALTESTS_HPP
#define _ES31FFUNCTIONALTESTS_HPP
/*-------------------------------------------------------------------------
 * drawElements Quality Program OpenGL ES 3.1 Module
 * -------------------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
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
 *//*!
 * \file
 * \brief Functional Tests.
 *//*--------------------------------------------------------------------*/

#include "tcuDefs.hpp"
#include "tes31TestCase.hpp"

namespace deqp
{
namespace gles31
{
namespace Functional
{

class GLES31FunctionalTests : public TestCaseGroup
{
public:
    GLES31FunctionalTests(Context &context);
    ~GLES31FunctionalTests(void);

    void init(void);

private:
    GLES31FunctionalTests(const GLES31FunctionalTests &other);
    GLES31FunctionalTests &operator=(const GLES31FunctionalTests &other);
};

class GL45FunctionalTests : public TestCaseGroup
{
public:
    GL45FunctionalTests(Context &context);
    ~GL45FunctionalTests(void);

    void init(void);

private:
    GL45FunctionalTests(const GL45FunctionalTests &other);
    GL45FunctionalTests &operator=(const GL45FunctionalTests &other);
};

} // namespace Functional
} // namespace gles31
} // namespace deqp

#endif // _ES31FFUNCTIONALTESTS_HPP
