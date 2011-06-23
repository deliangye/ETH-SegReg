/*
 * Grid.h
 *
 *  Created on: Nov 25, 2010
 *      Author: gasst
 */

#ifndef GRAPH_H
#define GRAPH_H
 
#include <vector>
#include <assert.h>
#include "itkConstNeighborhoodIterator.h"

using namespace std;
/*
 * Isotropic Graph
 * Returns current/next position in a grid based on size and resolution
 */
namespace itk{
template<class TImage, 
         class TUnaryRegistrationFunction, 
         class TPairwiseRegistrationFunction, 
         class TUnarySegmentationFunction, 
         class TPairwiseSegmentationRegistrationFunction,
         class TLabelMapper>
class GraphModel: public itk::Object{
public:
    typedef GraphModel Self;
   	typedef SmartPointer<Self>        Pointer;
	typedef SmartPointer<const Self>  ConstPointer;
    itkNewMacro(Self);

    //    typedef  itk::ImageToimageFilter<TImage,TImage> Superclass;
	typedef TImage ImageType;
    typedef typename TImage::IndexType IndexType;
	typedef typename TImage::OffsetType OffsetType;
	typedef typename TImage::PointType PointType;
	typedef typename TImage::SizeType SizeType;
	typedef typename TImage::SpacingType SpacingType;
	typedef typename TImage::Pointer ImagePointerType;
    typedef typename TImage::ConstPointer ConstImagePointerType;

    typedef typename itk::ConstNeighborhoodIterator<ImageType> ConstImageNeighborhoodIteratorType;

    typedef TUnaryRegistrationFunction UnaryRegistrationFunctionType;
    typedef typename UnaryRegistrationFunctionType::Pointer UnaryRegistrationFunctionPointerType;
    typedef TPairwiseRegistrationFunction PairwiseRegistrationFunctionType;
    typedef typename PairwiseRegistrationFunctionType::Pointer PairwiseRegistrationFunctionPointerType;
    typedef TUnarySegmentationFunction UnarySegmentationFunctionType;
    typedef typename UnarySegmentationFunctionType::Pointer UnarySegmentationFunctionPointerType;
    typedef TPairwiseSegmentationRegistrationFunction PairwiseSegmentationRegistrationFunctionType;
    typedef typename PairwiseSegmentationRegistrationFunctionType::Pointer PairwiseSegmentationRegistrationFunctionPointerType;
    
    typedef TLabelMapper LabelMapperType;
    typedef typename LabelMapperType::LabelType RegistrationLabelType;
	typedef typename itk::Image<RegistrationLabelType,ImageType::ImageDimension> RegistrationLabelImageType;
	typedef typename RegistrationLabelImageType::Pointer RegistrationLabelImagePointerType;

	typedef int SegmentationLabelType;
	typedef typename itk::Image<SegmentationLabelType,ImageType::ImageDimension> SegmentationLabelImageType;
	typedef typename SegmentationLabelImageType::Pointer SegmentationLabelImagePointerType;
    
protected:
    
	SizeType m_totalSize,m_imageLevelDivisors,m_graphLevelDivisors,m_gridSize, m_imageSize;
    
    //grid spacing in unit pixels
    SpacingType m_gridPixelSpacing;
    //grid spacing in mm
	SpacingType m_gridSpacing, m_imageSpacing;
    
    //spacing of the displacement labels
    SpacingType m_labelSpacing;

	PointType m_origin;
	double m_DisplacementScalingFactor;
	static const unsigned int m_dim=TImage::ImageDimension;
	int m_nNodes,m_nVertices, m_nRegistrationNodes, m_nSegmentationNodes;
	//ImageInterpolatorType m_ImageInterpolator,m_SegmentationInterpolator,m_BoneConfidenceInterploator;
	UnaryRegistrationFunctionPointerType m_unaryRegFunction;
	UnarySegmentationFunctionPointerType m_unarySegFunction;
    PairwiseSegmentationRegistrationFunctionPointerType m_pairwiseSegRegFunction;
    PairwiseRegistrationFunctionPointerType m_pairwiseRegFunction;
  
    //PairwiseFunctionPointerType m_pairwiseFunction;
	bool verbose;
	bool m_haveLabelMap;
    ConstImagePointerType m_fixedImage;
    ConstImageNeighborhoodIteratorType * m_fixedNeighborhoodIterator;

public:

    GraphModel(){
        assert(m_dim>1);
		assert(m_dim<4);
		m_haveLabelMap=false;
		verbose=true;
        m_fixedImage=NULL;
    };

    void setFixedImage(ConstImagePointerType fixedImage){
        m_fixedImage=fixedImage;
    }
    void setUpGraph(int nGraphNodesPerEdge){
        assert(m_fixedImage);

        //image size
		m_imageSize=m_fixedImage->GetLargestPossibleRegion().GetSize();
        m_imageSpacing=m_fixedImage->GetSpacing();
		std::cout<<"Full image resolution: "<<m_imageSize<<endl;
		m_nSegmentationNodes=1;
		m_nRegistrationNodes=1;
        //calculate graph spacing
		setSpacing(nGraphNodesPerEdge);
		if (LabelMapperType::nDisplacementSamples){
            
			m_labelSpacing=0.4*m_gridPixelSpacing/(LabelMapperType::nDisplacementSamples);
			if (verbose) std::cout<<"Spacing :"<<m_gridPixelSpacing<<" "<<LabelMapperType::nDisplacementSamples<<" labelSpacing :"<<m_labelSpacing<<std::endl;
		}
		for (int d=0;d<(int)m_dim;++d){
			if (verbose) std::cout<<"total size divided by spacing :"<<1.0*m_imageSize[d]/m_gridPixelSpacing[d]<<std::endl;

            //origin is original origin
			m_origin[d]=m_fixedImage->GetOrigin()[d];
            //
			m_gridSize[d]=m_imageSize[d]/m_gridPixelSpacing[d];
			if (m_gridSpacing!=1.0)
				m_gridSize[d]++;
            
			m_nRegistrationNodes*=m_gridSize[d];
            m_nSegmentationNodes*=m_imageSize[d];

            //level divisors are used to simplify the calculation of image indices from integer indices
			if (d>0){
				m_imageLevelDivisors[d]=m_imageLevelDivisors[d-1]*m_imageSize[d-1];
				m_graphLevelDivisors[d]=m_graphLevelDivisors[d-1]*m_gridSize[d-1];
			}else{
				m_imageLevelDivisors[d]=1;
				m_graphLevelDivisors[d]=1;
			}
		}
        m_nNodes=m_nRegistrationNodes+m_nSegmentationNodes;
		if (verbose) std::cout<<"GridSize: "<<m_dim<<" ";
        
        //nvertices is not used!?
		if (m_dim>=2){
			if (verbose) std::cout<<m_gridSize[0]<<" "<<m_gridSize[1];
			m_nVertices=m_gridSize[1]*(m_gridSize[0]-1)+m_gridSize[0]*(m_gridSize[1]-1);
		}
		if (m_dim==3){
			std::cout<<" "<<m_gridSize[2];
			m_nVertices=this->m_nVertices*this->m_gridSize[2]+(this->m_gridSize[2]-1)*this->m_gridSize[1]*this->m_gridSize[0];
		}
		if (verbose) std::cout<<" nodes:"<<m_nNodes<<" vertices:"<<m_nVertices<<" labels:"<<LabelMapperType::nLabels<<std::endl;
		//		m_ImageInterpolator.SetInput(m_movingImage);
        
        typename ConstImageNeighborhoodIteratorType::RadiusType r;
        for (int d=0;d<(int)m_dim;++d){
            r[d]=(m_gridPixelSpacing[d]/2);
        }
        m_fixedNeighborhoodIterator=new ConstImageNeighborhoodIteratorType(r,m_fixedImage,m_fixedImage->GetLargestPossibleRegion());
        if (verbose) std::cout<<" finished graph init" <<std::endl;
	}

	virtual void setSpacing(int divisor){
        assert(m_fixedImage);
		int minSpacing=999999;
		for (int d=0;d<ImageType::ImageDimension;++d){
			if(m_imageSize[d]/(divisor-1) < minSpacing){
				minSpacing=(m_imageSize[d]/(divisor-1)-1);
			}
		}
		minSpacing=minSpacing>=1?minSpacing:1.0;
		for (int d=0;d<ImageType::ImageDimension;++d){
			int div=m_imageSize[d]/minSpacing;
			div=div>0?div:1;
			double spacing=(1.0*m_imageSize[d]/div);
			if (spacing>1.0) spacing-=1.0/(div);
			m_gridPixelSpacing[d]=spacing;
			m_gridSpacing[d]=spacing*m_imageSpacing[d];
		}
 
	}
    


     
    //return position index in coarse graph from coarse graph node index
    virtual IndexType  getGraphIndex(int nodeIndex){
        IndexType position;
		for ( int d=m_dim-1;d>=0;--d){
            //position[d] is now the index in the coarse graph (image)
			position[d]=nodeIndex/m_graphLevelDivisors[d];
			nodeIndex-=position[d]*m_graphLevelDivisors[d];
		}
		return position;
    }
     
    //return position in full image from coarse graph node index
    virtual IndexType  getImageIndexFromCoarseGraphIndex(int idx){
        IndexType position;
		for ( int d=m_dim-1;d>=0;--d){
            //position[d] is now the index in the coarse graph (image)
			position[d]=idx/m_graphLevelDivisors[d];
			idx-=position[d]*m_graphLevelDivisors[d];
            //now calculate the fine image index from the coarse graph index
            position[d]*=m_gridSpacing[d]/m_imageSpacing[d];
		}
		return position;
    }
    virtual int  getGraphIntegerIndex(IndexType gridIndex){
        int i=0;
        for (unsigned int d=0;d<m_dim;++d){
            i+=gridIndex[d]*m_graphLevelDivisors[d];
        }
        return i;
    }
    virtual int  getImageIntegerIndex(IndexType imageIndex){
        int i=0;
        for (unsigned int d=0;d<m_dim;++d){
            i+=imageIndex[d]*m_imageLevelDivisors[d];
        }
        return i;
    }

    //return position in full image depending on fine graph nodeindex
     virtual IndexType getImageIndex(int idx){
        IndexType position;
		for ( int d=m_dim-1;d>=0;--d){
			position[d]=idx/m_imageLevelDivisors[d];
			idx-=position[d]*m_imageLevelDivisors[d];
		}
		return position;
    }
    double getUnaryRegistrationPotential(int nodeIndex,int labelIndex){
        IndexType imageIndex=getImageIndexFromCoarseGraphIndex(nodeIndex);
        RegistrationLabelType l=LabelMapperType::getLabel(labelIndex);
        l=LabelMapperType::scaleDisplacement(l,getDisplacementFactor());
        return m_unaryRegFunction->getPotential(imageIndex,l);
    }
    double getUnarySegmentationPotential(int nodeIndex,int labelIndex){
        IndexType imageIndex=getImageIndex(nodeIndex);
        //Segmentation:labelIndex==segmentationlabel
        double result=m_unarySegFunction->getPotential(imageIndex,labelIndex);
        if (result<0)
            std::cout<<imageIndex<<" " <<result<<std::endl;
        return result;
    };
    double getPairwiseRegistrationPotential(int nodeIndex1, int nodeIndex2, int labelIndex1, int labelIndex2){
        IndexType graphIndex1=getImageIndexFromCoarseGraphIndex(nodeIndex1);
        RegistrationLabelType l1=LabelMapperType::getLabel(labelIndex1);
        l1=LabelMapperType::scaleDisplacement(l1,getDisplacementFactor());
        IndexType graphIndex2=getImageIndexFromCoarseGraphIndex(nodeIndex2);
        RegistrationLabelType l2=LabelMapperType::getLabel(labelIndex2);
        l2=LabelMapperType::scaleDisplacement(l2,getDisplacementFactor());
        return m_pairwiseRegFunction->getPotential(graphIndex1, graphIndex2, l1,l2);
    };
    double getPairwiseSegRegPotential(int nodeIndex1, int nodeIndex2, int labelIndex1, int segmentationLabel){
        IndexType graphIndex=getImageIndexFromCoarseGraphIndex(nodeIndex1);
        IndexType imageIndex=getImageIndex(nodeIndex2);
        RegistrationLabelType registrationLabel=LabelMapperType::getLabel(labelIndex1);
        registrationLabel=LabelMapperType::scaleDisplacement(registrationLabel,getDisplacementFactor());
        return m_pairwiseSegRegFunction->getPotential(graphIndex,imageIndex,registrationLabel,segmentationLabel);
    }
    double getSegmentationWeight(int nodeIndex1, int nodeIndex2){
        IndexType imageIndex1=getImageIndex(nodeIndex1);
        IndexType imageIndex2=getImageIndex(nodeIndex2);
        double result=m_unarySegFunction->getWeight(imageIndex1,imageIndex2);
        if (result<0)
            std::cout<<imageIndex1<<" "<<imageIndex2<<" "<<result<<std::endl;
        return result;
    }
   
    
	std::vector<int> getForwardRegistrationNeighbours(int index){
		IndexType position=getGraphIndex(index);
		std::vector<int> neighbours;
		for ( int d=0;d<(int)m_dim;++d){
			OffsetType off;
			off.Fill(0);
			if ((int)position[d]<(int)m_gridSize[d]-1){
				off[d]+=1;
           
				neighbours.push_back(getGraphIntegerIndex(position+off));
			}
		}
		return neighbours;
	}
    std::vector<int> getForwardSegmentationNeighbours(int index){
		IndexType position=getImageIndex(index);
		std::vector<int> neighbours;
		for ( int d=0;d<(int)m_dim;++d){
			OffsetType off;
			off.Fill(0);
			if ((int)position[d]<(int)m_imageSize[d]-1){
				off[d]+=1;
				neighbours.push_back(getImageIntegerIndex(position+off));
			}
		}
		return neighbours;
	}
    std::vector<int>  getForwardSegRegNeighbours(int index){            
        IndexType imagePosition=getImageIndexFromCoarseGraphIndex(index);
		std::vector<int> neighbours;
        m_fixedNeighborhoodIterator->SetLocation(imagePosition);
        for (unsigned int i=0;i<m_fixedNeighborhoodIterator->Size();++i){
            IndexType idx=m_fixedNeighborhoodIterator->GetIndex(i);
            if (m_fixedImage->GetLargestPossibleRegion().IsInside(idx)){
                neighbours.push_back(getImageIntegerIndex(idx));
            }
        }
        return neighbours;
    }
    
    RegistrationLabelImagePointerType getDeformationImage(std::vector<int>  labels){
        RegistrationLabelImagePointerType result=RegistrationLabelImageType::New();
        typename RegistrationLabelImageType::RegionType region;
        region.SetSize(m_gridSize);
        result->SetRegions(region);
        result->SetSpacing(m_gridSpacing);
        result->SetDirection(m_fixedImage->GetDirection());
        result->SetOrigin(m_origin);
        result->Allocate();
        typename itk::ImageRegionIterator<RegistrationLabelImageType> it(result,region);
        int i=0;
        for (it.GoToBegin();!it.IsAtEnd();++it,++i){
            assert(i<labels.size());
            RegistrationLabelType l=LabelMapperType::getLabel(labels[i]);
            l=LabelMapperType::scaleDisplacement(l,getDisplacementFactor());
            it.Set(l);
        }
        assert(i==(labels.size()));
        return result;
    }
    ImagePointerType getSegmentationImage(std::vector<int> labels){
        ImagePointerType result=ImageType::New();
        result->SetRegions(m_fixedImage->GetLargestPossibleRegion());
        result->SetSpacing(m_fixedImage->GetSpacing());
        result->SetDirection(m_fixedImage->GetDirection());
        result->SetOrigin(m_fixedImage->GetOrigin());
        result->Allocate();
        typename itk::ImageRegionIterator<ImageType> it(result,result->GetLargestPossibleRegion());
        int i=0;
        for (it.GoToBegin();!it.IsAtEnd();++it,++i){
            assert(i<labels.size());
            it.Set(labels[i]);
        }
        assert(i==(labels.size()));
        return result;
    }
	SizeType getImageSize() const
	{
		return m_imageSize;
	}
	SizeType getGridSize() const
	{return m_gridSize;}

	int nNodes(){return m_nNodes;}
	int nVertices(){return m_nVertices;}

	ImagePointerType getFixedImage(){
		return m_fixedImage;
	}
    void setUnaryRegistrationFunction(UnaryRegistrationFunctionPointerType unaryFunc){
        m_unaryRegFunction=unaryFunc;
    }
	void setUnarySegmentationFunction(UnarySegmentationFunctionPointerType func){
        m_unarySegFunction=func;
    }
    void setPairwiseSegmentationRegistrationFunction( PairwiseSegmentationRegistrationFunctionPointerType func){
        m_pairwiseSegRegFunction=func;
    }
    void setPairwiseRegistrationFunction( PairwiseRegistrationFunctionPointerType func){
        m_pairwiseRegFunction=func;
    }

	typename ImageType::DirectionType getDirection(){return m_fixedImage->GetDirection();}

	void setDisplacementFactor(double fac){m_DisplacementScalingFactor=fac;}
	SpacingType getDisplacementFactor(){return m_labelSpacing*m_DisplacementScalingFactor;}
	SpacingType getSpacing(){return m_gridSpacing;}	
    SpacingType getPixelSpacing(){return m_gridPixelSpacing;}
	PointType getOrigin(){return m_origin;}
    int nRegNodes(){
        return m_nRegistrationNodes;
    }
    int nSegNodes(){
        return m_nSegmentationNodes;
    }
    int nRegLabels(){
        return LabelMapperType::nDisplacements;
    }
    int nSegLabels(){
        return LabelMapperType::nSegmentations;
    }
}; //GraphModel

}//namespace

#endif /* GRIm_dim_H_ */
