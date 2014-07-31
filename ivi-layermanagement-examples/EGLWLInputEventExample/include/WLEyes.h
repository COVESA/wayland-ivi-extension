/**************************************************************************
 *
 * Copyright 2010, 2011 BMW Car IT GmbH
 * Copyright (C) 2011 DENSO CORPORATION and Robert Bosch Car Multimedia Gmbh
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/
#ifndef _WLEYES_H_
#define _WLEYES_H_

#include "transform.h"

//////////////////////////////////////////////////////////////////////////////

class WLEye
{
private:
    int     m_nPoint;
    TPoint* m_eyeLiner;
    int     m_nPupilPoint;
    TPoint* m_pupil;
    int     m_nWhiteEyePoint;
    TPoint* m_whiteEye;

public:
    WLEye();
    virtual ~WLEye();

    void CreateEyeLiner(const float& centerX,
                        const float& centerY,
                        const float& diam,
                        const Transform& trans);
    void CreatePupil(const float& centerX,
                     const float& centerY,
                     const float& diam,
                     const Transform& trans);

    void GetEyeLinerGeom(int* nPoint, float** points);
    void GetWhiteEyeGeom(int* nPoint, float** points);
    void GetPupilGeom(int* nPoint, float** points);

private:
    void CreateWhiteEye(const float& centerX,
                        const float& centerY,
                        const float& diam,
                        const Transform& trans);
};

//////////////////////////////////////////////////////////////////////////////

class WLEyes
{
private:
    WLEye*    m_eyes[2];
    int       m_width;
    int       m_height;
    Transform m_trans;

public:
    WLEyes(int screenWidth, int screenHeight);
    virtual ~WLEyes();

    void SetPointOfView(int mousePosX, int mousePosY);

    void GetEyeLinerGeom(int n, int* nPoint, float** points);
    void GetWhiteEyeGeom(int n, int* nPoint, float** points);
    void GetPupilGeom(int n, int* nPoint, float** points);

private:
    WLEyes();
};

#endif /* _WLEYES_H_ */
