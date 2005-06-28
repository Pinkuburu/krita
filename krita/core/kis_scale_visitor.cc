/*
 *  Copyright (c) 2004, 2005 Michael Thaler <michael.thaler@physik.tu-muenchen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <qdatetime.h>

#include <kdebug.h>
#include <klocale.h>

#include "kis_paint_device.h"
#include "kis_scale_visitor.h"
#include "kis_progress_display_interface.h"

double KisSimpleScaleFilterStrategy::valueAt(double t) const {
        /* f(t) = 2|t|^3 - 3|t|^2 + 1, -1 <= t <= 1 */
        if(t < 0.0) t = -t;
        if(t < 1.0) return((2.0 * t - 3.0) * t * t + 1.0);
        return(0.0);
}

double KisBoxScaleFilterStrategy::valueAt(double t) const {
        if((t > -0.5) && (t <= 0.5)) return(1.0);
        return(0.0);
}

double KisTriangleScaleFilterStrategy::valueAt(double t) const {
        if(t < 0.0) t = -t;
        if(t < 1.0) return(1.0 - t);
        return(0.0);
}

double KisBellScaleFilterStrategy::valueAt(double t) const {
        if(t < 0) t = -t;
        if(t < .5) return(.75 - (t * t));
        if(t < 1.5) {
                t = (t - 1.5);
                return(.5 * (t * t));
        }
        return(0.0);
}

double KisBSplineScaleFilterStrategy::valueAt(double t) const {
        double tt;

        if(t < 0) t = -t;
        if(t < 1) {
                tt = t * t;
                return((.5 * tt * t) - tt + (2.0 / 3.0));
        } else if(t < 2) {
                t = 2 - t;
                return((1.0 / 6.0) * (t * t * t));
        }
        return(0.0);
}

double KisLanczos3ScaleFilterStrategy::valueAt(double t) const {
        if(t < 0) t = -t;
        if(t < 3.0) return(sinc(t) * sinc(t/3.0));
        return(0.0);
}

double KisLanczos3ScaleFilterStrategy::sinc(double x) const {
        const double pi=3.1415926535897932385;
        x *= pi;
        if(x != 0) return(sin(x) / x);
        return(1.0);
}

double KisMitchellScaleFilterStrategy::valueAt(double t) const {
        const double B=1.0/3.0;
        const double C=1.0/3.0;
        double tt;

        tt = t * t;
        if(t < 0) t = -t;
        if(t < 1.0) {
                t = (((12.0 - 9.0 * B - 6.0 * C) * (t * tt)) + ((-18.0 + 12.0 * B + 6.0 * C) * tt) + (6.0 - 2 * B));
                return(t / 6.0);
        } else if(t < 2.0) {
                t = (((-1.0 * B - 6.0 * C) * (t * tt)) + ((6.0 * B + 30.0 * C) * tt) + ((-12.0 * B - 48.0 * C) * t) + (8.0 * B + 24 * C));
                return(t / 6.0);
                }
        return(0.0);
}


void KisScaleVisitor::scale(double xscale, double yscale, KisProgressDisplayInterface *m_progress, enumFilterType filterType)
{
	
	double fwidth = 0;

        KisScaleFilterStrategy *filterStrategy = 0;

        switch(filterType){
                case BOX_FILTER:
                        filterStrategy = new KisBoxScaleFilterStrategy();
                        break;
                case TRIANGLE_FILTER:
                        filterStrategy = new KisTriangleScaleFilterStrategy();
                        break;
                case BELL_FILTER:
                        filterStrategy = new KisBellScaleFilterStrategy();
                        break;
                case B_SPLINE_FILTER:
                        filterStrategy = new KisBSplineScaleFilterStrategy();
                        break;
                case FILTER:
                        filterStrategy = new KisSimpleScaleFilterStrategy();
                        break;
                case LANCZOS3_FILTER:
                        filterStrategy = new KisLanczos3ScaleFilterStrategy();
                        break;
                case MITCHELL_FILTER:
                        filterStrategy = new KisMitchellScaleFilterStrategy();
                        break;
        }

        fwidth = filterStrategy->support();

        // target image data
        Q_INT32 targetW;
        Q_INT32 targetH;

        Q_INT32 width = m_dev->image()->width();
        Q_INT32 height = m_dev->image()->height();
        m_pixelSize=m_dev -> pixelSize();

        // compute size of target image
        if ( xscale == 1.0F && yscale == 1.0F ) {
                return;
        }
        targetW = QABS( qRound( xscale * width ) );
        targetH = QABS( qRound( yscale * height ) );
	
        QUANTUM * newData = new QUANTUM[targetW * targetH * m_pixelSize ];
	Q_CHECK_PTR(newData);

        double weight[ m_pixelSize ];	/* filter calculation variables */

        QUANTUM *pel = new QUANTUM[ m_pixelSize ];
	Q_CHECK_PTR(pel);

        QUANTUM *pel2 = new QUANTUM[ m_pixelSize ];
	Q_CHECK_PTR(pel2);

        bool bPelDelta[ m_pixelSize ];
        ContribList	*contribX;
        ContribList	contribY;
        const Q_INT32 BLACK_PIXEL=0;
        const Q_INT32 WHITE_PIXEL=255;


        // create intermediate row to hold vertical dst row zoom
        QUANTUM * tmp = new QUANTUM[ width * m_pixelSize ];
	Q_CHECK_PTR(tmp);

	//create array of pointers to intermediate rows
	QUANTUM **tmpRows = new QUANTUM*[ height ];
	
        //create array of pointers to intermediate rows that are actually used simultaneously and allocate memory for the rows
        QUANTUM **tmpRowsMem = new QUANTUM*[ (int)(fwidth * 2 + 1) ];
        for(int i = 0; i < (int)(fwidth * 2 + 1); i++)
	{
                tmpRowsMem[i] = new QUANTUM[ width * m_pixelSize ];
	        Q_CHECK_PTR(tmpRowsMem[i]);
        }

        //progress info
        m_cancelRequested = false;
        m_progress -> setSubject(this, true, true);
	emit notifyProgressStage(this,i18n("Scaling layer..."),0);
		
        // build x weights
        contribX = new ContribList[ targetW ];
        for(int x = 0; x < targetW; x++)
        {
                calcContrib(&contribX[x], xscale, fwidth, width, filterStrategy, x);
        }

	QTime starttime = QTime::currentTime ();
	
        for(int y = 0; y < targetH; y++)
        {
                //progress info
                emit notifyProgress(this,(y * 100) / targetH);
                if (m_cancelRequested) {
                        break;
                }

                // build y weights
                calcContrib(&contribY, yscale, fwidth, height, filterStrategy, y);
                
		//copy pixel data to temporary arrays
		for(int srcpos = 0; srcpos < contribY.n; srcpos++)
		{
			if (!(contribY.p[srcpos].m_pixel < 0 || contribY.p[srcpos].m_pixel >= height))
			{
				
				tmpRows[contribY.p[srcpos].m_pixel] = new QUANTUM[ width * m_pixelSize * sizeof( QUANTUM ) ];
				//tmpRows[ contribY.p[srcpos].m_pixel ] = tmpRowsMem[ srcpos ];
				m_dev -> readBytes(tmpRows[contribY.p[srcpos].m_pixel], 0, contribY.p[srcpos].m_pixel, width, 1);
			}
		}
		
		/* Apply vert filter to make dst row in tmp. */
                for(int x = 0; x < width; x++)
                {
                        for(int channel = 0; channel < m_pixelSize; channel++){
                                weight[channel] = 0.0;
                                bPelDelta[channel] = FALSE;
                        	pel[channel]=tmpRows[contribY.p[0].m_pixel][ x * m_pixelSize + channel ];
			}
			//m_dev -> readBytes(pel, x, contribY.p[0].m_pixel, 1, 1);
                        for(int srcpos = 0; srcpos < contribY.n; srcpos++)
                        {
				if (!(contribY.p[srcpos].m_pixel < 0 || contribY.p[srcpos].m_pixel >= height)){
                                        //m_dev -> readBytes(pel2, x, contribY.p[srcpos].m_pixel, 1, 1);
                                        //kdDebug() << "y: " << y << " x: " << x << " contribY.p[srcpos].m_pixel: " << contribY.p[srcpos].m_pixel << endl;
					for(int channel = 0; channel < m_pixelSize; channel++)
                                        {
                                                pel2[channel]=tmpRows[contribY.p[srcpos].m_pixel][ x * m_pixelSize + channel ];
						if(pel2[channel] != pel[channel]) bPelDelta[channel] = TRUE;
                                                weight[channel] += pel2[channel] * contribY.p[srcpos].m_weight;
                                        }
                                }
                        }

                        for(int channel = 0; channel < m_pixelSize; channel++){
                                weight[channel] = bPelDelta[channel] ? static_cast<int>(qRound(weight[channel])) : pel[channel];
                                tmp[ x * m_pixelSize + channel ] = static_cast<QUANTUM>(CLAMP(weight[channel], BLACK_PIXEL, WHITE_PIXEL));
                        }
                } /* next row in temp column */
                delete[] contribY.p;
		
		//delete the temporary rows
		for(int srcpos = 0; srcpos < contribY.n; srcpos++)
		{
			if (!(contribY.p[srcpos].m_pixel < 0 || contribY.p[srcpos].m_pixel >= height))
			{
				if(!tmpRows[contribY.p[srcpos].m_pixel])
				{	
					delete[] tmpRows[contribY.p[srcpos].m_pixel];
				}
			}
		}
                /* The temp column has been built. Now stretch it
                vertically into dst column. */
                for(int x = 0; x < targetW; x++)
                {
                        for(int channel = 0; channel < m_pixelSize; channel++){
                                weight[channel] = 0.0;
                                bPelDelta[channel] = FALSE;
                                pel[channel] = tmp[ contribX[x].p[0].m_pixel * m_pixelSize + channel ];
                        }
                        for(int srcpos = 0; srcpos < contribX[x].n; srcpos++)
                        {
                                for(int channel = 0; channel < m_pixelSize; channel++){
                                        pel2[channel] = tmp[ contribX[x].p[srcpos].m_pixel * m_pixelSize + channel ];
                                        if(pel2[channel] != pel[channel]) bPelDelta[channel] = TRUE;
                                        weight[channel] += pel2[channel] * contribX[x].p[srcpos].m_weight;
                                }
                        }
                        for(int channel = 0; channel < m_pixelSize; channel++){
                                weight[channel] = bPelDelta[channel] ? static_cast<int>(qRound(weight[channel])) : pel[channel];
                                int currentPos = (y*targetW+x) * m_pixelSize; // try to be at least a little efficient
                                if (weight[channel]<0) newData[currentPos + channel] = 0;
                                else if (weight[channel]>255) newData[currentPos + channel] = 255;
                                else newData[currentPos + channel] = (uchar)weight[channel];
                       }
                } /* next dst row */
        } /* next dst column */
        if(!m_cancelRequested){
                m_dev -> writeBytes( newData, 0, 0, targetW, targetH);
                m_dev -> crop(0, 0, targetW, targetH);
        }

        /* free the memory allocated for horizontal filter weights */
        for(int x = 0; x < targetW; x++)
                delete[] contribX[x].p;
        delete[] contribX;

        delete filterStrategy;
        delete[] newData;
        delete[] pel;
        delete[] pel2;
        delete[] tmp;
	
        //progress info
        emit notifyProgressDone(this);

	QTime stoptime = QTime::currentTime ();
	kdDebug() << "time needed for scaling: " << starttime.msecsTo ( stoptime )  << "ms" << endl; 
	
        return;
}

int KisScaleVisitor::calcContrib(ContribList *contrib, double scale, double fwidth, int srcwidth, KisScaleFilterStrategy* filterStrategy, Q_INT32 i)
{
        //ContribList* contribX: receiver of contrib info
        //double xscale: horizontal zooming scale
        //double fwidth: Filter sampling width
        //int dstwidth: Target bitmap width
        //int srcwidth: Source bitmap width
        //double (*filterf)(double): Filter proc
        //int i: Pixel column in source bitmap being processed

        double width;
        double fscale;
        double center, begin, end;
        double weight;
        Q_INT32 k, n;

        if(scale < 1.0)
        {
                //Shrinking image 
                width = fwidth / scale;
                fscale = 1.0 / scale;

                contrib->n = 0;
                contrib->p = new Contrib[ (int)(width * 2 + 1) ];

                center = (double) i / scale;
                begin = ceil(center - width);
                end = floor(center + width);
                for(int srcpos = (int)begin; srcpos <= end; ++srcpos)
                {
                        weight = center - (double) srcpos;
                        weight = filterStrategy->valueAt(weight / fscale) / fscale;
                        if(srcpos < 0)
                                n = -srcpos;
                        else if(srcpos >= srcwidth)
                                n = (srcwidth - srcpos) + srcwidth - 1;
                        else
                                n = srcpos;

                        k = contrib->n++;
                        contrib->p[k].m_pixel = n;
                        contrib->p[k].m_weight = weight;
                }
        }
        else
        {
                // Expanding image
                contrib->n = 0;
                contrib->p = new Contrib[ (int)(fwidth * 2 + 1) ];
                
                center = (double) i / scale;
                begin = ceil(center - fwidth);
                end = floor(center + fwidth);

                for(int srcpos = (int)begin; srcpos <= end; ++srcpos)
                {
                        weight = center - (double) srcpos;
                        weight = filterStrategy->valueAt(weight);
                        if(srcpos < 0) {
                                n = -srcpos;
                        } else if(srcpos >= srcwidth) {
                                n = (srcwidth - srcpos) + srcwidth - 1;
                        } else {
                                n = srcpos;
                        }
                        k = contrib->n++;
                        contrib->p[k].m_pixel = n;
                        contrib->p[k].m_weight = weight;
                }
        }
        return 0;
} /* calc_x_contrib */
