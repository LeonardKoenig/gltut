// MousePole.cpp: implementation of the MousePole class.
//
//////////////////////////////////////////////////////////////////////

#include <GL/freeglut.h>
#include "framework.h"
#include "MousePole.h"
#include "MatrixStack.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265f
#endif

#define PI_2 1.570796325f

glm::vec3 g_UnitZ(0.0f, 0.0f, 1.0f);

namespace Framework
{
	MousePole::MousePole(glm::vec3 target, const RadiusDef &radiusDef)
		: m_lookAt(target)
		, m_radCurrXZAngle(0.0)
		, m_radCurrYAngle(-PI_2 / 2.0f)
		, m_radCurrSpin(0.0f)
		, m_fRadius(20.0f)
		, m_radius(radiusDef)
		, m_bIsDragging(false)
	{}

	MousePole::~MousePole()
	{
	}

	namespace
	{
		glm::mat4 CalcLookAtMatrix(const glm::vec3 &cameraPt,
			const glm::vec3 &lookPt, const glm::vec3 &upPt)
		{
			glm::vec3 lookDir = glm::normalize(lookPt - cameraPt);
			glm::vec3 upDir = glm::normalize(upPt);

			glm::vec3 rightDir = glm::normalize(glm::cross(lookDir, upDir));
			glm::vec3 perpUpDir = glm::cross(rightDir, lookDir);

			glm::mat4 rotMat(1.0f);
			rotMat[0] = glm::vec4(rightDir, 0.0f);
			rotMat[1] = glm::vec4(perpUpDir, 0.0f);
			rotMat[2] = glm::vec4(-lookDir, 0.0f);

			rotMat = glm::transpose(rotMat);

			glm::mat4 transMat(1.0f);
			transMat[3] = glm::vec4(-cameraPt, 1.0f);

			return rotMat * transMat;
		}
	}

	glm::mat4 MousePole::CalcMatrix() const
	{
		//Compute the position vector along the xz plane.
		float cosa = cosf(m_radCurrXZAngle);
		float sina = sinf(m_radCurrXZAngle);

		glm::vec3 currPos(-sina, 0.0f, cosa);

		//Compute the "up" rotation axis.
		//This axis is a 90 degree rotation around the y axis. Just a component-swap and negate.
		glm::vec3 UpRotAxis;

		UpRotAxis.x = currPos.z;
		UpRotAxis.y = currPos.y;
		UpRotAxis.z = -currPos.x;

		//Now, rotate around this axis by the angle.
		Framework::MatrixStack theStack;

		theStack.SetIdentity();
		theStack.RotateRadians(UpRotAxis, m_radCurrYAngle);
		currPos = glm::vec3(theStack.Top() * glm::vec4(currPos, 0.0));

		//Set the position of the camera.
		glm::vec3 tempVec = currPos * m_radius.fCurrRadius;
		glm::vec3 cameraPosition = tempVec + m_lookAt;

		//Now, compute the up-vector.
		//The direction of the up-vector is the cross-product of currPos and UpRotAxis.
		//Rotate this vector around the currPos axis given m_currSpin.

		glm::vec3 upVec = glm::cross(currPos, UpRotAxis);

		theStack.SetIdentity();
		theStack.RotateRadians(currPos, m_radCurrSpin);
		upVec = glm::vec3(theStack.Top() * glm::vec4(upVec, 0.0));

		return CalcLookAtMatrix(cameraPosition, m_lookAt, upVec);
	}


	void MousePole::BeginDragRotate(const glm::ivec2 &ptStart, RotateMode rotMode)
	{
		m_RotateMode = rotMode;

		m_initialPt = ptStart;

		m_radInitXZAngle = m_radCurrXZAngle;
		m_radInitYAngle = m_radCurrYAngle;
		m_radInitSpin = m_radCurrSpin;

		m_bIsDragging = true;
	}

	void MousePole::OnDragRotate(const glm::ivec2 &ptCurr)
	{
		glm::ivec2 iDiff = ptCurr - m_initialPt;

		switch(m_RotateMode)
		{
		case RM_DUAL_AXIS_ROTATE:
			ProcessXChange(iDiff.x);
			ProcessYChange(iDiff.y);
			break;
		case RM_XZ_AXIS_ROTATE:
			ProcessXChange(iDiff.x);
			break;
		case RM_Y_AXIS_ROTATE:
			ProcessYChange(iDiff.y);
			break;
		case RM_SPIN_VIEW_AXIS:
			ProcessSpinAxis(iDiff.x, iDiff.y);
		default:
			break;
		}
	}
#define SCALE_FACTOR 250

	const float X_CONVERSION_FACTOR = PI_2 / SCALE_FACTOR;
	const float Y_CONVERSION_FACTOR = PI_2 / SCALE_FACTOR;
	const float SPIN_CONV_FACTOR = PI_2 / SCALE_FACTOR;

	void MousePole::ProcessXChange(int iXDiff)
	{
		m_radCurrXZAngle = (iXDiff * X_CONVERSION_FACTOR) + m_radInitXZAngle;
	}

	void MousePole::ProcessYChange(int iYDiff)
	{
		m_radCurrYAngle = (-iYDiff * Y_CONVERSION_FACTOR) + m_radInitYAngle;
	}

	void MousePole::ProcessSpinAxis(int iXDiff, int iYDiff)
	{
		m_radCurrSpin = (iXDiff * SPIN_CONV_FACTOR) + m_radInitSpin;
	}


	void MousePole::EndDragRotate(const glm::ivec2 &ptEnd, bool bKeepResults)
	{
		if(bKeepResults)
		{
			OnDragRotate(ptEnd);
		}
		else
		{
			m_radCurrXZAngle = m_radInitXZAngle;
			m_radCurrYAngle = m_radInitYAngle;
		}

		m_bIsDragging = false;
	}

	void MousePole::MoveCloser( bool bLargeStep /*= true*/ )
	{
		if(bLargeStep)
			m_radius.fCurrRadius -= m_radius.fLargeDelta;
		else
			m_radius.fCurrRadius -= m_radius.fSmallDelta;

		if(m_radius.fCurrRadius < m_radius.fMinRadius)
			m_radius.fCurrRadius = m_radius.fMinRadius;
	}

	void MousePole::MoveAway( bool bLargeStep /*= true*/ )
	{
		if(bLargeStep)
			m_radius.fCurrRadius += m_radius.fLargeDelta;
		else
			m_radius.fCurrRadius += m_radius.fSmallDelta;

		if(m_radius.fCurrRadius > m_radius.fMaxRadius)
			m_radius.fCurrRadius = m_radius.fMaxRadius;
	}

	void MousePole::GLUTMouseMove( const glm::ivec2 &position )
	{
		if(m_bIsDragging)
			OnDragRotate(position);
	}

	void MousePole::GLUTMouseButton( int button, int btnState, const glm::ivec2 &position )
	{
		if(btnState == GLUT_DOWN)
		{
			//Ignore all other button presses when dragging.
			if(!m_bIsDragging)
			{
				switch(button)
				{
				case GLUT_LEFT_BUTTON:
					if(glutGetModifiers() & GLUT_ACTIVE_CTRL)
						this->BeginDragRotate(position, MousePole::RM_XZ_AXIS_ROTATE);
					else
						this->BeginDragRotate(position, MousePole::RM_DUAL_AXIS_ROTATE);
					break;
				case GLUT_MIDDLE_BUTTON:
					this->BeginDragRotate(position, MousePole::RM_SPIN_VIEW_AXIS);
					break;
				case GLUT_RIGHT_BUTTON:
					this->BeginDragRotate(position, MousePole::RM_Y_AXIS_ROTATE);
					break;
				default:
					break;
				}
			}
		}
		else
		{
			//Ignore all other button releases when not dragging
			if(m_bIsDragging)
			{
				switch(button)
				{
				case GLUT_LEFT_BUTTON:
					if(m_RotateMode == MousePole::RM_XZ_AXIS_ROTATE ||
						m_RotateMode == MousePole::RM_DUAL_AXIS_ROTATE)
						this->EndDragRotate(position);
					break;
				case GLUT_MIDDLE_BUTTON:
					if(m_RotateMode == MousePole::RM_SPIN_VIEW_AXIS)
						this->EndDragRotate(position);
					break;
				case GLUT_RIGHT_BUTTON:
					if(m_RotateMode == MousePole::RM_Y_AXIS_ROTATE)
						this->EndDragRotate(position);
					break;
				default:
					break;
				}
			}
		}
	}

	void MousePole::GLUTMouseWheel( int direction, const glm::ivec2 &position )
	{
		if(direction > 0)
			this->MoveCloser(!(glutGetModifiers() & GLUT_ACTIVE_SHIFT));
		else
			this->MoveAway(!(glutGetModifiers() & GLUT_ACTIVE_SHIFT));
	}
}
