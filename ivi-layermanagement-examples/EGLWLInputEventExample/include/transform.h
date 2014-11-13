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
#ifndef _TRANSFORM_H_
#define _TRANSFORM_H_

typedef struct _transform {
    float mx, bx;
    float my, by;
} Transform;

typedef struct _tpoint {
    float x, y;
} TPoint;

typedef struct _trectangle {
    float x, y, width, height;
} TRectangle;

#define Xx(x,y,t)  ((int)((t)->mx * (x) + (t)->bx + 0.5))
#define Xy(x,y,t)  ((int)((t)->my * (y) + (t)->by + 0.5))
#define Xwidth(w,h,t)  ((int)((t)->mx * (w) + 0.5))
#define Xheight(w,h,t) ((int)((t)->my * (h) + 0.5))
#define Tx(x,y,t)  ((((float) (x)) - (t)->bx) / (t)->mx)
#define Ty(x,y,t)  ((((float) (y)) - (t)->by) / (t)->my)
#define Twidth(w,h,t)  (((float) (w)) / (t)->mx)
#define Theight(w,h,t) (((float) (h)) / (t)->my)

#endif
