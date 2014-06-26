/***************************************************************************
*
* Copyright 2010,2011 BMW Car IT GmbH
* Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
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
#ifndef _ILM_COMMON_H_
#define _ILM_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "ilm_types.h"

/**
 * \brief Initializes the IVI LayerManagement Client.
 * \ingroup ilmCommon
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if a connection can not be established to the services.
 */
ilmErrorTypes ilm_init(void);

/**
 * \brief Initializes the IVI LayerManagement Client.
 * \ingroup ilmCommon
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if a connection can not be established to the services.
 */
ilmErrorTypes ilm_initWithNativedisplay(t_ilm_nativedisplay nativedisplay);

/**
 * \brief Returns initialization state of the IVI LayerManagement Client.
 * \ingroup ilmCommon
 * \return true if client library is initialized
 * \return false if client library is not initialized
 */
t_ilm_bool ilm_isInitialized(void);

/**
 * \brief Commit all changes and execute all enqueued commands since last commit.
 * \ingroup ilmCommon
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not call the method on the service.
 */
ilmErrorTypes ilm_commitChanges(void);

/**
 * \brief Destroys the IVI LayerManagement Client.
 * \ingroup ilmCommon
 * \return ILM_SUCCESS if the method call was successful
 * \return ILM_FAILED if the client can not be closed or was not initialized.
 */
ilmErrorTypes ilm_destroy(void);



#ifdef __cplusplus
} /**/
#endif /* __cplusplus */

#endif /* _ILM_COMMON_H_ */
