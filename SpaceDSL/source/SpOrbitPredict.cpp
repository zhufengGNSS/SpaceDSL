﻿/************************************************************************
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
* Date:2018-03-20
* Description:
*   SpOrbitPredict.cpp
*
*   Purpose:
*
*       Orbit Prediction Algorithm and Function
*
*
*   Last modified:
*
*   2018-03-27  Niu Zhiyong (1st edition)
*
*************************************************************************/

#include "SpaceDSL/SpOrbitPredict.h"
#include "SpaceDSL/SpCoordSystem.h"
#include "SpaceDSL/SpMath.h"
#include "SpaceDSL/SpUtils.h"



namespace SpaceDSL {

    /*************************************************
     * Class type: Normalization Parameter
     * Author: Niu ZhiYong
     * Date:2018-03-20
     * Description:
    **************************************************/
    NormalizeParameter::NormalizeParameter()
    {

    }

    NormalizeParameter::~NormalizeParameter()
    {

    }

    double NormalizeParameter::GetLengthPara(SolarSysStarType centerStarType)
    {
        switch (centerStarType)
        {
        case E_Earth:
            return EarthRadius;
        case E_Sun:
            return AU;
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "NormalizeParameter: Center Star Type Unsupport ");
        }
    }

    double NormalizeParameter::GetSpeedPara(SolarSysStarType centerStarType)
    {
        switch (centerStarType)
        {
        case E_Earth:
            return sqrt(GM_Earth/EarthRadius);
        case E_Sun:
            return sqrt(GM_Sun/AU);
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "NormalizeParameter: Center Star Type Unsupport ");
        }
    }

    double NormalizeParameter::GetTimePara(SolarSysStarType centerStarType)
    {
        switch (centerStarType)
        {
        case E_Earth:
            return EarthRadius/sqrt(GM_Earth/EarthRadius);
        case E_Sun:
            return AU/sqrt(GM_Sun/AU);
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "NormalizeParameter: Center Star Type Unsupport ");
        }
    }

    double NormalizeParameter::GetThrustPara(SolarSysStarType centerStarType, double mass)
    {
        switch (centerStarType)
        {
        case E_Earth:
            return  (GM_Earth*mass)/pow(EarthRadius,2);
        case E_Sun:
            return (GM_Sun*mass)/pow(AU,2);
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "NormalizeParameter: Center Star Type Unsupport ");
        }
    }


    /*************************************************
     * Class type: Orbit Prediction Parameters
     * Author: Niu ZhiYong
     * Date:2018-03-20
     * Description:
     *  Orbit Prediction Algorithm and Function
    **************************************************/
    ThirdBodyGravitySign OrbitPredictConfig::DefaultThirdBodySign = ThirdBodyGravitySign();

    OrbitPredictConfig::OrbitPredictConfig()
    {
        m_pGravityModel     = nullptr;
        m_pGeodeticSystem   = nullptr;
        m_pThirdBodyGrva    = nullptr;
        m_pAtmosphericDrag  = nullptr;
        m_pSolarRadPressure = nullptr;
    }

    OrbitPredictConfig::~OrbitPredictConfig()
    {
        if (m_pGravityModel != nullptr)
            delete m_pGravityModel;

        if (m_pGeodeticSystem != nullptr)
            delete m_pGeodeticSystem;

        if (m_pThirdBodyGrva != nullptr)
            delete m_pThirdBodyGrva;

        if (m_pAtmosphericDrag != nullptr)
            delete m_pAtmosphericDrag;

        if (m_pSolarRadPressure != nullptr)
            delete m_pSolarRadPressure;
    }

    void OrbitPredictConfig::Initializer(double Mjd_UTC, SolarSysStarType centerStarType, bool isUseNormalize,
                                         GravityModel::GravModelType gravModelType, int maxDegree, int maxOrder,
                                         ThirdBodyGravitySign thirdBodyGravSign, GeodeticCoordSystem::GeodeticCoordType geodeticType,
                                         AtmosphereModel::AtmosphereModelType atmModelType, double dragCoef, double dragArea,
                                         double f107A, double f107, double ap[],
                                         double SRPCoef, double SRPArea, bool isUseDrag, bool isUseSRP)
    {
        if ( m_bIsInitialized == true)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig Has Initialized! ");

        m_CenterStarType    = centerStarType;

        m_MJD_UTC           = Mjd_UTC;

        m_TAI_UTC           = m_IERSService.GetValue(Mjd_UTC, "leapseconds");//TAI-UTC
        m_UT1_UTC           = m_IERSService.GetValue(Mjd_UTC, "UT1-UTC");
        m_TT_UTC            = IERSService::TT_TAI + m_TAI_UTC;

        m_MJD_TT            = Mjd_UTC + m_TT_UTC/DayToSec;
        m_MJD_TAI           = m_MJD_TT - IERSService::TT_TAI/DayToSec;
        m_MJD_UT1           = Mjd_UTC + m_UT1_UTC/DayToSec;

        m_X_Pole            = m_IERSService.GetValue(Mjd_UTC, "x_pole");
        m_Y_Pole            = m_IERSService.GetValue(Mjd_UTC, "y_pole");

        m_GravModelType     = gravModelType;
        m_MaxDegree         = maxDegree;
        m_MaxOrder          = maxOrder;

        m_AtmModelType      = atmModelType;
        m_DragCoef          = dragCoef;
        m_DragArea          = dragArea;
        m_F107A             = f107A;
        m_F107              = f107;
        m_Ap                = ap;
        m_SRPCoef           = SRPCoef;
        m_SRPArea           = SRPArea;

        m_bIsUseDrag        = isUseDrag;
        m_bIsUseSRP         = isUseSRP;
        m_ThirdBodySign     = thirdBodyGravSign;
        m_GeodeticCoordType = geodeticType;
        m_bIsUseNormalize   = isUseNormalize;

        //Data Cheak
        switch (centerStarType)
        {
        case E_Mercury:
            m_ThirdBodySign.bIsUseMercuryGrav = false;
            break;
        case E_Venus:
            m_ThirdBodySign.bIsUseVenusGrav = false;
            break;
        case E_Earth:
            m_ThirdBodySign.bIsUseEarthGrav = false;
            break;
        case E_Mars:
            m_ThirdBodySign.bIsUseMarsGrav = false;
            break;
        case E_Jupiter:
            m_ThirdBodySign.bIsUseJupiterGrav = false;
            break;
        case E_Saturn:
            m_ThirdBodySign.bIsUseSaturnGrav = false;
            break;
        case E_Uranus:
            m_ThirdBodySign.bIsUseUranusGrav = false;
            break;
        case E_Neptune:
            m_ThirdBodySign.bIsUseNeptuneGrav = false;
            break;
        case E_Pluto:
            m_ThirdBodySign.bIsUsePlutoGrav = false;
            break;
        case E_Moon:
            m_ThirdBodySign.bIsUseMoonGrav = false;
            break;
        case E_Sun:
            m_ThirdBodySign.bIsUseSunGrav = false;
            break;
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: SolarSysStarType Unsupport ");
        }

        if (gravModelType != GravityModel::GravModelType::E_NotDefinedGravModel)
            m_pGravityModel = new GravityModel(gravModelType);

        if (geodeticType != GeodeticCoordSystem::GeodeticCoordType::E_NotDefinedGeodeticType)
            m_pGeodeticSystem = new GeodeticCoordSystem(geodeticType);

        if (IsUseThirdBodyGravity() == true)
        {
            m_pThirdBodyGrva = new ThirdBodyGravity();
            m_pThirdBodyGrva->SetCenterStar(centerStarType);
        }

        if (m_bIsUseDrag == true)
        {
            m_pAtmosphericDrag = new AtmosphericDrag(atmModelType, m_pGeodeticSystem);
        }

        if (m_bIsUseSRP == true)
        {
            m_pSolarRadPressure = new SolarRadPressure();
        }

        m_bIsInitialized = true;
    }

    bool OrbitPredictConfig::IsInitialized()
    {
        return m_bIsInitialized;
    }

    void OrbitPredictConfig::Update(double Mjd_UTC)
    {
        this->SetMJD_UTC(Mjd_UTC);
    }

    void OrbitPredictConfig::SetCenterStarType(SolarSysStarType type)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_CenterStarType = type;
    }

    SolarSysStarType OrbitPredictConfig::GetCenterStarType() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_CenterStarType;
    }

    double OrbitPredictConfig::GetCenterStarGM() const
    {
        switch (m_CenterStarType)
        {
        case E_Mercury:
            return GM_Mercury;
        case E_Venus:
            return GM_Venus;
        case E_Earth:
            return GM_Earth;
        case E_Mars:
            return GM_Mars;
        case E_Jupiter:
            return GM_Jupiter;
        case E_Saturn:
            return GM_Saturn;
        case E_Uranus:
            return GM_Uranus;
        case E_Neptune:
            return GM_Neptune;
        case E_Pluto:
            return GM_Pluto;
        case E_Moon:
            return GM_Moon;
        case E_Sun:
            return GM_Sun;
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,"OrbitPredictConfig:SolarSysStarType Unsupport ");
        }
    }

    void OrbitPredictConfig::SetMJD_UTC(double Mjd_UTC)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_MJD_UTC           = Mjd_UTC;

        m_TAI_UTC           = m_IERSService.GetValue(Mjd_UTC, "leapseconds");//TAI-UTC
        m_UT1_UTC           = m_IERSService.GetValue(Mjd_UTC, "UT1-UTC");
        m_TT_UTC            = IERSService::TT_TAI + m_TAI_UTC;

        m_MJD_TT            = Mjd_UTC + m_TT_UTC/DayToSec;
        m_MJD_TAI           = m_MJD_TT - IERSService::TT_TAI/DayToSec;
        m_MJD_UT1           = Mjd_UTC + m_UT1_UTC/DayToSec;
    }

    double OrbitPredictConfig::GetMJD_UTC() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_MJD_UTC;
    }

    double OrbitPredictConfig::GetMJD_UT1() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_MJD_UT1;
    }

    double OrbitPredictConfig::GetMJD_TT() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_MJD_TT;
    }

    double OrbitPredictConfig::GetMJD_TAI() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_MJD_TAI;
    }

    double OrbitPredictConfig::GetTAI_UTC() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_TAI_UTC;
    }

    double OrbitPredictConfig::GetUT1_UTC() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_UT1_UTC;
    }

    double OrbitPredictConfig::GetTT_UTC() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_TT_UTC;
    }

    double OrbitPredictConfig::GetX_Pole() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_X_Pole;
    }

    double OrbitPredictConfig::GetY_Pole() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_Y_Pole;
    }

    void OrbitPredictConfig::SetGravModelType(GravityModel::GravModelType type)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_GravModelType = type;
    }

    GravityModel::GravModelType OrbitPredictConfig::GetGravModelType() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_GravModelType;
    }

    void OrbitPredictConfig::SetGravMaxDegree(int maxDegree)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_MaxDegree = maxDegree;
    }

    int OrbitPredictConfig::GetGravMaxDegree() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_MaxDegree;
    }

    void OrbitPredictConfig::SetGravMaxOrder(int maxOrder)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_MaxOrder = maxOrder;
    }

    int OrbitPredictConfig::GetGravMaxOrder() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_MaxOrder;
    }

    void OrbitPredictConfig::SetAtmosphereModelType(AtmosphereModel::AtmosphereModelType type)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_AtmModelType = type;
    }

    AtmosphereModel::AtmosphereModelType OrbitPredictConfig::GetAtmosphereModelType() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_AtmModelType;
    }

    void OrbitPredictConfig::SetDragCoef(double coef)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_DragCoef = coef;
    }

    double OrbitPredictConfig::GetDragCoef() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_DragCoef;
    }

    void OrbitPredictConfig::SetDragArea(double area)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_DragArea = area;
    }

    double OrbitPredictConfig::GetDragArea() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_DragArea;
    }

    void OrbitPredictConfig::SetAverageF107(double f107A)
    {
        m_F107A = f107A;
    }

    double OrbitPredictConfig::GetAverageF107() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_F107A;
    }

    void OrbitPredictConfig::SetDailyF107(double f107)
    {
        m_F107 = f107;
    }

    double OrbitPredictConfig::GetDailyF107() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_F107;
    }

    void OrbitPredictConfig::SetGeomagneticIndex(double ap[])
    {
        m_Ap = ap;
    }

    double *OrbitPredictConfig::GetGeomagneticIndex() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_Ap;
    }

    void OrbitPredictConfig::SetSRPCoef(double coef)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_SRPCoef = coef;
    }

    double OrbitPredictConfig::GetSRPCoef() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_SRPCoef;
    }

    void OrbitPredictConfig::SetSRPArea(double area)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_SRPArea = area;
    }

    double OrbitPredictConfig::GetSRPArea() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_SRPArea;
    }

    void OrbitPredictConfig::SetThirdBodySign(ThirdBodyGravitySign sign)
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        m_ThirdBodySign = sign;
        //Data Cheak
        switch (m_CenterStarType)
        {
        case E_Mercury:
            m_ThirdBodySign.bIsUseMercuryGrav = false;
            break;
        case E_Venus:
            m_ThirdBodySign.bIsUseVenusGrav = false;
            break;
        case E_Earth:
            m_ThirdBodySign.bIsUseEarthGrav = false;
            break;
        case E_Mars:
            m_ThirdBodySign.bIsUseMarsGrav = false;
            break;
        case E_Jupiter:
            m_ThirdBodySign.bIsUseJupiterGrav = false;
            break;
        case E_Saturn:
            m_ThirdBodySign.bIsUseSaturnGrav = false;
            break;
        case E_Uranus:
            m_ThirdBodySign.bIsUseUranusGrav = false;
            break;
        case E_Neptune:
            m_ThirdBodySign.bIsUseNeptuneGrav = false;
            break;
        case E_Pluto:
            m_ThirdBodySign.bIsUsePlutoGrav = false;
            break;
        case E_Moon:
            m_ThirdBodySign.bIsUseMoonGrav = false;
            break;
        case E_Sun:
            m_ThirdBodySign.bIsUseSunGrav = false;
            break;
        default:
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: SolarSysStarType Unsupport ");
        }

    }

    ThirdBodyGravitySign OrbitPredictConfig::GetThirdBodySign() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_ThirdBodySign;
    }

    void OrbitPredictConfig::SetGeodeticCoordType(GeodeticCoordSystem::GeodeticCoordType geodeticType)
    {
        m_GeodeticCoordType = geodeticType;
    }

    GeodeticCoordSystem::GeodeticCoordType OrbitPredictConfig::GetGeodeticCoordType() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_GeodeticCoordType;
    }

    bool OrbitPredictConfig::IsUseThirdBodyGravity() const
    {
        if (m_ThirdBodySign.bIsUseEarthGrav == true ||
            m_ThirdBodySign.bIsUseJupiterGrav ==true ||
            m_ThirdBodySign.bIsUseMarsGrav ==true ||
            m_ThirdBodySign.bIsUseMercuryGrav ==true ||
            m_ThirdBodySign.bIsUseMoonGrav ==true ||
            m_ThirdBodySign.bIsUseNeptuneGrav ==true ||
            m_ThirdBodySign.bIsUsePlutoGrav ==true ||
            m_ThirdBodySign.bIsUseSaturnGrav ==true ||
            m_ThirdBodySign.bIsUseSunGrav ==true ||
            m_ThirdBodySign.bIsUseUranusGrav ==true ||
            m_ThirdBodySign.bIsUseVenusGrav ==true )

            return true;
        else
            return false;
    }

    bool OrbitPredictConfig::IsUseSRP() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_bIsUseSRP;
    }

    bool OrbitPredictConfig::IsUseDrag() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_bIsUseDrag;
    }

    bool OrbitPredictConfig::IsUseNormalize() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_bIsUseNormalize;
    }


    GravityModel *OrbitPredictConfig::GetGravityModel() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_pGravityModel;
    }

    GeodeticCoordSystem *OrbitPredictConfig::GetGeodeticCoordSystem() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");

        return m_pGeodeticSystem;
    }

    ThirdBodyGravity *OrbitPredictConfig::GetThirdBodyGravity() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_pThirdBodyGrva;
    }

    AtmosphericDrag *OrbitPredictConfig::GetAtmosphericDrag() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_pAtmosphericDrag;
    }

    SolarRadPressure *OrbitPredictConfig::GetSolarRadPressure() const
    {
        if ( m_bIsInitialized == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictConfig: m_OrbitPredictConfig UnInitialized! ");
        return m_pSolarRadPressure;
    }

    /*************************************************
     * Class type: Orbit Prediction Right Function
     * Author: Niu ZhiYong
     * Date:2018-03-20
     * Description:
    **************************************************/
    OrbitPredictRightFunc::OrbitPredictRightFunc()
    {
        m_pOrbitPredictConfig   = nullptr;
        m_pGravityModel         = nullptr;
        m_pGeodeticSystem       = nullptr;
        m_pThirdBodyGrva        = nullptr;
        m_pAtmosphericDrag      = nullptr;
        m_pSolarRadPressure     = nullptr;
    }

    OrbitPredictRightFunc::OrbitPredictRightFunc(OrbitPredictConfig *pConfig)
    {
        m_pOrbitPredictConfig = pConfig;

        m_pGravityModel = m_pOrbitPredictConfig->GetGravityModel();
        if (m_pGravityModel == nullptr)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictRightFunc: GravityModel == nullptr! ");

        m_pGeodeticSystem = m_pOrbitPredictConfig->GetGeodeticCoordSystem();
        if (m_pGeodeticSystem == nullptr)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictRightFunc: GeodeticSystem == nullptr! ");

        m_pThirdBodyGrva = m_pOrbitPredictConfig->GetThirdBodyGravity();

        m_pAtmosphericDrag = m_pOrbitPredictConfig->GetAtmosphericDrag();

        m_pSolarRadPressure = m_pOrbitPredictConfig->GetSolarRadPressure();
    }

    OrbitPredictRightFunc::~OrbitPredictRightFunc()
    {

    }

    void OrbitPredictRightFunc::SetOrbitPredictConfig(OrbitPredictConfig *pConfig)
    {
        m_pOrbitPredictConfig = pConfig;

        m_pGravityModel = m_pOrbitPredictConfig->GetGravityModel();
        if (m_pGravityModel == nullptr)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictRightFunc: GravityModel == nullptr! ");

        m_pGeodeticSystem = m_pOrbitPredictConfig->GetGeodeticCoordSystem();
        if (m_pGeodeticSystem == nullptr)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictRightFunc: GeodeticSystem == nullptr! ");

        m_pThirdBodyGrva = m_pOrbitPredictConfig->GetThirdBodyGravity();

        m_pAtmosphericDrag = m_pOrbitPredictConfig->GetAtmosphericDrag();

        m_pSolarRadPressure = m_pOrbitPredictConfig->GetSolarRadPressure();
    }

    void OrbitPredictRightFunc::operator()(double t, const VectorXd &x, VectorXd &result) const
    {
        double Mjd_TT = t * SecToDay;
        Vector3d acceleration;
        acceleration.fill(0);
        Vector3d pos;
        pos(0) = x(0);  pos(1) = x(1);  pos(2) = x(2);
        Vector3d vel;
        vel(0) = x(3);  vel(1) = x(4);  vel(2) = x(5);
        double Mjd_UTC = Mjd_TT - m_pOrbitPredictConfig->GetTT_UTC()/DayToSec;
        Matrix3d J2000toECFMtx = GeodeticCoordSystem::GetJ2000ToECFMtx(Mjd_UTC);

        /// Harmonic Gravity
        acceleration += m_pGravityModel->AccelHarmonicGravity(pos, J2000toECFMtx,
                                                            m_pOrbitPredictConfig->GetGravMaxDegree(),
                                                            m_pOrbitPredictConfig->GetGravMaxOrder());
        /// Atmospheric Drag
        if (m_pOrbitPredictConfig->IsUseDrag())
        {

            double Mjd_UT1 = Mjd_TT + (m_pOrbitPredictConfig->GetUT1_UTC() - m_pOrbitPredictConfig->GetTT_UTC())/DayToSec;
            Vector3d dragAccel = m_pAtmosphericDrag->AccelAtmosphericDrag(  Mjd_UTC,Mjd_UT1,
                                                                            pos, vel,
                                                                            m_pOrbitPredictConfig->GetDragArea(),
                                                                            m_pOrbitPredictConfig->GetDragCoef(),
                                                                            x(6),
                                                                            m_pOrbitPredictConfig->GetAverageF107(),
                                                                            m_pOrbitPredictConfig->GetDailyF107(),
                                                                            m_pOrbitPredictConfig->GetGeomagneticIndex());
            //cout<<Mjd_UTC<<"  "<<dragAccel(0)<<"  "<<dragAccel(1)<<"  "<<dragAccel(2)<<endl;
            acceleration += dragAccel;
        }

        /// Solar Rad. Pressure
        if (m_pOrbitPredictConfig->IsUseSRP())
        {
            acceleration += m_pSolarRadPressure->AccelSolarRad( Mjd_TT, pos,
                                                                m_pOrbitPredictConfig->GetSRPArea(),
                                                                m_pOrbitPredictConfig->GetDragCoef(),
                                                                x(6));
        }

        /// Third Body Gravity
        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseEarthGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Earth);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseJupiterGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Jupiter);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseMarsGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Mars);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseMercuryGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Mercury);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseMoonGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Moon);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseNeptuneGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Neptune);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUsePlutoGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Pluto);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseSaturnGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Saturn);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseSunGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Sun);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseUranusGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Uranus);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->GetThirdBodySign().bIsUseVenusGrav)
        {
            m_pThirdBodyGrva->SetThirdBodyStar(E_Venus);
            acceleration += m_pThirdBodyGrva->AccelPointMassGravity(Mjd_TT, pos);
        }

        if (m_pOrbitPredictConfig->IsUseNormalize())
        {
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredictRightFunc: High Precision Orbit Prediction does not Support Normalize! ");
        }
        /// make the right function
        for (int i = 0; i < 3; ++i)
        {
            result(i) = vel(i);
        }
        result(3) = acceleration(0);
        result(4) = acceleration(1);
        result(5) = acceleration(2);
        result(6) = 0;
    }

    /*************************************************
     * Class type: Orbit Prediction
     * Author: Niu ZhiYong
     * Date:2018-03-20
     * Description:
     *  Orbit Prediction Algorithm and Function
    **************************************************/
    OrbitPredict::OrbitPredict()
    {
        m_pRightFunc = nullptr;
        m_pRungeKutta = nullptr;
    }

    OrbitPredict::~OrbitPredict()
    {
        if (m_pRightFunc != nullptr)
            delete m_pRightFunc;

        if (m_pRungeKutta != nullptr)
            delete m_pRungeKutta;
    }

    void OrbitPredict::OrbitStep(OrbitPredictConfig &predictConfig, double step, IntegMethodType integType,
                                 double &mass, Vector3d &pos, Vector3d &vel)
    {
        if (predictConfig.IsInitialized() == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "OrbitPredict: m_OrbitPredictConfig UnInitialized! ");

        double Mjd_TT = predictConfig.GetMJD_TT();
        VectorXd x(7), result(7);
        result.fill(0);
        x(0) = pos(0);  x(1) = pos(1);  x(2) = pos(2);
        x(3) = vel(0);  x(4) = vel(1);  x(5) = vel(2);  x(6) = mass;

        if (m_pRightFunc == nullptr)
            m_pRightFunc = new OrbitPredictRightFunc(&predictConfig);
        else
            m_pRightFunc->SetOrbitPredictConfig(&predictConfig);

        if (m_pRungeKutta == nullptr)
            m_pRungeKutta = new RungeKutta(integType);
        else
            m_pRungeKutta->SetIntegMethodType(integType);

        m_pRungeKutta->OneStep(m_pRightFunc, Mjd_TT*DayToSec ,x, step, result);

        pos(0) = result(0);     pos(1) = result(1);     pos(2) = result(2);
        vel(0) = result(3);     vel(1) = result(4);     vel(2) = result(5);
        mass   = result(6);

    }



    /*************************************************
     * Class type: Tow Body Orbit Prediction
     * Author: Niu ZhiYong
     * Date:2018-03-20
     * Description:
     *  Orbit Prediction Algorithm and Function
    **************************************************/
    TwoBodyOrbitPredict::TwoBodyOrbitPredict()
    {
        m_pRightFunc = nullptr;
        m_pRungeKutta = nullptr;
    }

    TwoBodyOrbitPredict::~TwoBodyOrbitPredict()
    {
        if (m_pRightFunc != nullptr)
            delete m_pRightFunc;

        if (m_pRungeKutta != nullptr)
            delete m_pRungeKutta;
    }

    void TwoBodyOrbitPredict::OrbitStep(OrbitPredictConfig &predictConfig, double step, IntegMethodType integType,
                                                  double &mass, Vector3d &pos, Vector3d &vel)
    {
        if (predictConfig.IsInitialized() == false)
            throw SPException(__FILE__, __FUNCTION__, __LINE__,
                              "TwoBodyOrbitPredict: m_OrbitPredictConfig UnInitialized! ");
        if (predictConfig.IsUseNormalize())
        {
            SolarSysStarType centerStarType = predictConfig.GetCenterStarType();
            double Mjd_TT = predictConfig.GetMJD_TT()/NormalizeParameter::GetTimePara(centerStarType);
            double norm_step = step/NormalizeParameter::GetTimePara(centerStarType);
            VectorXd x(7), result(7);
            result.fill(0);
            x(0) = pos(0)/NormalizeParameter::GetLengthPara(centerStarType);
            x(1) = pos(1)/NormalizeParameter::GetLengthPara(centerStarType);
            x(2) = pos(2)/NormalizeParameter::GetLengthPara(centerStarType);

            x(3) = vel(0)/NormalizeParameter::GetSpeedPara(centerStarType);
            x(4) = vel(1)/NormalizeParameter::GetSpeedPara(centerStarType);
            x(5) = vel(2)/NormalizeParameter::GetSpeedPara(centerStarType);

            x(6) = mass;

            if (m_pRightFunc == nullptr)
                m_pRightFunc = new TwoBodyOrbitRightFunc(&predictConfig);
            else
                m_pRightFunc->SetOrbitPredictConfig(&predictConfig);

            if (m_pRungeKutta == nullptr)
                m_pRungeKutta = new RungeKutta(integType);
            else
                m_pRungeKutta->SetIntegMethodType(integType);

            m_pRungeKutta->OneStep(m_pRightFunc, Mjd_TT*DayToSec ,x, norm_step, result);

            pos(0) = result(0)*NormalizeParameter::GetLengthPara(centerStarType);
            pos(1) = result(1)*NormalizeParameter::GetLengthPara(centerStarType);
            pos(2) = result(2)*NormalizeParameter::GetLengthPara(centerStarType);

            vel(0) = result(3)*NormalizeParameter::GetSpeedPara(centerStarType);
            vel(1) = result(4)*NormalizeParameter::GetSpeedPara(centerStarType);
            vel(2) = result(5)*NormalizeParameter::GetSpeedPara(centerStarType);

            mass   = result(6);
        }
        else
        {
            double Mjd_TT = predictConfig.GetMJD_TT();
            VectorXd x(7), result(7);
            result.fill(0);
            x(0) = pos(0);  x(1) = pos(1);  x(2) = pos(2);
            x(3) = vel(0);  x(4) = vel(1);  x(5) = vel(2);  x(6) = mass;

            TwoBodyOrbitRightFunc rightFunc(&predictConfig);
            RungeKutta RK(integType);
            RK.OneStep(&rightFunc, Mjd_TT*DayToSec ,x, step, result);

            pos(0) = result(0);     pos(1) = result(1);     pos(2) = result(2);
            vel(0) = result(3);     vel(1) = result(4);     vel(2) = result(5);
            mass   = result(6);
        }

    }

    /*************************************************
     * Class type: Tow Body Orbit Prediction Right Function
     * Author: Niu ZhiYong
     * Date:2018-03-20
     * Description:
    **************************************************/
    TwoBodyOrbitRightFunc::TwoBodyOrbitRightFunc()
    {
        m_pOrbitPredictConfig = nullptr;
    }

    TwoBodyOrbitRightFunc::TwoBodyOrbitRightFunc(OrbitPredictConfig *pConfig)
    {
        m_pOrbitPredictConfig = pConfig;
    }

    TwoBodyOrbitRightFunc::~TwoBodyOrbitRightFunc()
    {

    }

    void TwoBodyOrbitRightFunc::SetOrbitPredictConfig(OrbitPredictConfig *pConfig)
    {
        m_pOrbitPredictConfig = pConfig;
    }

    void TwoBodyOrbitRightFunc::operator()(double t, const VectorXd &x, VectorXd &result) const
    {
        Vector3d pos;
        pos(0) = x(0);  pos(1) = x(1);  pos(2) = x(2);
        Vector3d vel;
        vel(0) = x(3);  vel(1) = x(4);  vel(2) = x(5);
        double r = pos.norm();
        if (m_pOrbitPredictConfig->IsUseNormalize())
        {
            /// make the right function
            for (int i = 0; i < 3; ++i)
            {
                result(i) = vel(i);
            }
            result(3) = -pos(0)/pow(r,3);
            result(4) = -pos(1)/pow(r,3);
            result(5) = -pos(2)/pow(r,3);
            result(6) = 0;
        }
        else
        {
            double GM = m_pOrbitPredictConfig->GetCenterStarGM();
            /// make the right function
            for (int i = 0; i < 3; ++i)
            {
                result(i) = vel(i);
            }
            result(3) = -GM*pos(0)/pow(r,3);
            result(4) = -GM*pos(1)/pow(r,3);
            result(5) = -GM*pos(2)/pow(r,3);
            result(6) = 1;
        }


    }

}

