/************************************************************************
* Copyright (C) 2018 Niu ZhiYong
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Author: Niu ZhiYong
* Date:2018-07-27
* Description:
*   SpCZMLScript.h
*
*   Purpose:
*
*        Read and Write CZML File by Jsoncpp
*
*
*   Last modified:
*
*   2018-07-27  Niu Zhiyong (1st edition)
*
*************************************************************************/

#ifndef SPCZMLSCRIPT_H
#define SPCZMLSCRIPT_H

#include "SpaceDSL_Global.h"

#include <Jsoncpp/json.hpp>

#include <string>
#include <vector>

using namespace std;
using namespace nlohmann;
/// All the functions are in the namespace SpaceDSL
///
namespace SpaceDSL {

    class Mission;
    /*************************************************
     * Class type: The class of SpaceDSL Moderator
     * Author: Niu ZhiYong
     * Date:2018-07-27
     * Description:
    **************************************************/
    class SPACEDSL_API CZMLScript
    {
    public:
        explicit CZMLScript();
        ~CZMLScript();

    public:

         void Initializer(const string &filePath, const Mission *pMission = nullptr, const double step = 300);

        /********************************************************************/
        /// Writing CZML Content For Use With Cesium.
        /// @Author	Niu Zhiyong
        /// @Date	2018-07-29
        /// @Input
        /// @Param	filePath
        /// @Return void
        /********************************************************************/
        void        WirteCZML();

        /********************************************************************/
        /// Writing CZML Content For Use With Cesium.
        /// @Author	xiaogongwei
        /// @Date	2018-11-06
        /// @Input  data_MJD_POS=[mjd1,x1,y1,z1,mjd2,x2,y2,z2,......] (x1,y1,z1) is position in J2000 coordinate system
        /// @Input  data_MJD_Vel=[mjd1,v1,v1,v1,mjd2,v2,v2,v2,......] (v1,v1,v1) is speed
        /// @vehicl_name  this is you defined vehicl name
        /// @Return void
        /********************************************************************/
        void WirteCZML(vector<double> data_MJD_POS, vector<double> data_MJD_Vel, string vehicl_name = "Vehicl Defualt Name");

    private:

        string      FormTimeStr(int year, int month, int day, int hour, int min, double sec);

        string      FormTimeIntervalStr(int year0, int month0, int day0, int hour0, int min0, double sec0,
                                           int year1, int month1, int day1, int hour1, int min1, double sec1);

    private:
        bool                m_bIsInitialized;
        vector<json>        *m_pJsonList;
        json                *m_pJsonToWirte;
        string              m_FilePath;
        const Mission   	*m_pMission;
        double              m_Step;




    };


}


#endif //SPCZMLSCRIPT_H
