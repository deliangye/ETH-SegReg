/*
 * Potentials.h
 *
 *  Created on: Nov 24, 2010
 *      Author: gasst
 */

#ifndef _REGISTRATIONUNARYPOTENTIAL_H_
#define _REGISTRATIONUNARYPOTENTIAL_H_
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <utility>
#include "itkVector.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkConstNeighborhoodIterator.h"
namespace itk{



    template<class TLabelMapper,class TImage>
    class UnaryPotentialRegistration : public itk::Object{
    public:
        //itk declarations
        typedef UnaryPotentialRegistration            Self;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;

        typedef	TImage ImageType;
        typedef typename ImageType::Pointer ImagePointerType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;

        typedef TLabelMapper LabelMapperType;
        typedef typename LabelMapperType::LabelType LabelType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::SizeType SizeType;
        typedef typename ImageType::SpacingType SpacingType;
        typedef LinearInterpolateImageFunction<ImageType> InterpolatorType;
        typedef typename InterpolatorType::Pointer InterpolatorPointerType;
        typedef typename InterpolatorType::ContinuousIndexType ContinuousIndexType;
        typedef typename LabelMapperType::LabelImagePointerType LabelImagePointerType;
        typedef typename itk::ConstNeighborhoodIterator<ImageType> ImageNeighborhoodIteratorType;
        typedef typename ImageNeighborhoodIteratorType::RadiusType RadiusType;

        SizeType m_fixedSize,m_movingSize;
    protected:
        ConstImagePointerType m_fixedImage, m_movingImage;
        InterpolatorPointerType m_movingInterpolator;
        LabelImagePointerType m_baseLabelMap;
        bool m_haveLabelMap;
        bool radiusSet;
        RadiusType m_radius;
        ImageNeighborhoodIteratorType * nIt;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(RegistrationUnaryPotential, Object);

        UnaryPotentialRegistration(){
            m_haveLabelMap=false;
            radiusSet=false;
        }
        
        
        virtual void freeMemory(){
        }
        void SetRadius(SpacingType sp){
            for (int d=0;d<ImageType::ImageDimension;++d){
                m_radius[d]=sp[d];
            }
            radiusSet=true;
        }
        void SetRadius(RadiusType sp){
            m_radius=sp;
            radiusSet=true;
        }
        void SetBaseLabelMap(LabelImagePointerType blm){m_baseLabelMap=blm;m_haveLabelMap=true;}
        LabelImagePointerType GetBaseLabelMap(LabelImagePointerType blm){return m_baseLabelMap;}
        void SetMovingInterpolator(InterpolatorPointerType movingImage){
            m_movingInterpolator=movingImage;
        }
    	void SetMovingImage(ConstImagePointerType movingImage){
            m_movingImage=movingImage;
            m_movingSize=m_movingImage->GetLargestPossibleRegion().GetSize();
        }
        void SetFixedImage(ConstImagePointerType fixedImage){
            m_fixedImage=fixedImage;
            m_fixedSize=m_fixedImage->GetLargestPossibleRegion().GetSize();
            assert(radiusSet);
            nIt=new ImageNeighborhoodIteratorType(this->m_radius,this->m_fixedImage, this->m_fixedImage->GetLargestPossibleRegion());
        }
        
        virtual double getPotential(IndexType fixedIndex, LabelType disp){
            double result=0;
            itk::Vector<float,ImageType::ImageDimension> baseDisp=m_baseLabelMap->GetPixel(fixedIndex);
            nIt->SetLocation(fixedIndex);
            double count=0;
            double sff=0.0,smm=0.0,sfm=0.0,sf=0.0,sm=0.0;
            for (unsigned int i=0;i<nIt->Size();++i){
                bool inBounds;
                double f=nIt->GetPixel(i,inBounds);
                if (inBounds){
                    IndexType neighborIndex=nIt->GetIndex(i);
                    //this should be weighted somehow
                    ContinuousIndexType idx2(neighborIndex);
                    double weight=1.0;
                    idx2+=disp+this->m_baseLabelMap->GetPixel(neighborIndex);
                    //                    cout<<fixedIndex<<" "<<disp<<" "<<idx2<<" "<<endl;
                    double m;
                    if (!this->m_movingInterpolator->IsInsideBuffer(idx2)){
                        continue;
                        m=0;
#if 0
                        for (int d=0;d<ImageType::ImageDimension;++d){
                            if (idx2[d]>=this->m_movingInterpolator->GetEndContinuousIndex()[d]){
                                idx2[d]=this->m_movingInterpolator->GetEndContinuousIndex()[d]-0.5;
                            }
                            else if (idx2[d]<this->m_movingInterpolator->GetStartContinuousIndex()[d]){
                                idx2[d]=this->m_movingInterpolator->GetStartContinuousIndex()[d]+0.5;
                            }
                        }
#endif
                    }else{
                        m=this->m_movingInterpolator->EvaluateAtContinuousIndex(idx2);
                    }
                    //       cout<<f<<" "<<m<<" "<<sff<<" "<<sfm<<" "<<sf<<" "<<sm<<endl;
                    sff+=f*f;
                    smm+=m*m;
                    sfm+=f*m;
                    sf+=f;
                    sm+=m;
                    count+=1;
                }

            }
            //cout<<"doneit"<<endl;
            if (count){
                //                cout<<" "<<sff<<" "<<sfm<<" "<<smm<<" "<<sf<<" "<<sm<<" "<<count<<" "<<sff- ( sf * sf / count )<<endl;

                sff -= ( sf * sf / count );
                smm -= ( sm * sm / count );
                sfm -= ( sf * sm / count );
                //                cout<<" "<<sff<<" "<<sfm<<" "<<smm<<" "<<sf<<" "<<sm<<" "<<count<<" "<<sff- ( sf * sf / count )<<endl;
                double result;
                if (smm*sff>0){
                    result=1-(1.0*sfm/sqrt(smm*sff)/2);
                    //  result=(1-fabs(1.0*sfm/sqrt(smm*sff)));
                    //  result=(1-(1.0*sfm/sqrt(smm*sff)+1.0)/2);

                }
                else if (sfm>0)result=0;
                else result=1;
                return result;
            }
            //no correlation whatsoever
            else return 0.5;
            return result;
        }
    };//class

    template<class TLabelMapper,class TImage>
    class UnaryPotentialRegistrationSAD : public UnaryPotentialRegistration<TLabelMapper, TImage>{
    public:
        //itk declarations
        typedef UnaryPotentialRegistrationSAD           Self;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;
          typedef	TImage ImageType;
        typedef typename ImageType::Pointer ImagePointerType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;

        typedef TLabelMapper LabelMapperType;
        typedef typename LabelMapperType::LabelType LabelType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::SizeType SizeType;
        typedef typename ImageType::SpacingType SpacingType;
        typedef LinearInterpolateImageFunction<ImageType> InterpolatorType;
        typedef typename InterpolatorType::Pointer InterpolatorPointerType;
        typedef typename InterpolatorType::ContinuousIndexType ContinuousIndexType;
        typedef typename LabelMapperType::LabelImagePointerType LabelImagePointerType;
        typedef typename itk::ConstNeighborhoodIterator<ImageType> ImageNeighborhoodIteratorType;
        typedef typename ImageNeighborhoodIteratorType::RadiusType RadiusType;

    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(RegistrationUnaryPotentialSAD, Object);

        UnaryPotentialRegistrationSAD(){}
        
        virtual double getPotential(IndexType fixedIndex, LabelType disp){
            double result=0;
            itk::Vector<float,ImageType::ImageDimension> baseDisp=this->m_baseLabelMap->GetPixel(fixedIndex);
            this->nIt->SetLocation(fixedIndex);
            double count=0;
            double sum=0.0;
            for (unsigned int i=0;i<this->nIt->Size();++i){
                bool inBounds;
                double f=this->nIt->GetPixel(i,inBounds);
                if (inBounds){
                    IndexType neighborIndex=this->nIt->GetIndex(i);
                    //this should be weighted somehow
                    ContinuousIndexType idx2(neighborIndex);
                    double weight=1.0;
                    idx2+=disp+this->m_baseLabelMap->GetPixel(neighborIndex);
                    //                    cout<<fixedIndex<<" "<<disp<<" "<<idx2<<" "<<endl;
                    double m;
                    if (!this->m_movingInterpolator->IsInsideBuffer(idx2)){
                        continue;
                        m=0;
                    }else{
                        m=this->m_movingInterpolator->EvaluateAtContinuousIndex(idx2);
                    }
                    result+=fabs(m-f);
                    count+=1;
                }

            }

            if (count)
                return result/count;
            else
                return 999999999;
        }
    };//class
}//namespace
#endif /* POTENTIALS_H_ */
