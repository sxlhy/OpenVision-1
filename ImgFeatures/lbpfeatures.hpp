#ifndef LBPFEATURES_H
#define LBPFEATURES_H
#include "Common/OpenCVCommon.hpp"
#include "ImgFeatures/integralImage.h"
#include "ImgProc/Histogram.hpp"
/**
 * @brief The LBPFeatures class :encapsulates the methods to compute the local binary pattern
 *image as well as spatial LBP histogram
 */
class LBPFeatures
{
public:
    /**
     * @brief LBPFeatures : constructor for LBP features
     */
    LBPFeatures();

    //input image
    Mat image;
    //vector of LBP features
    vector<uchar> features;
    Mat mask;
    //integral image class
    IntegralImage ix;
    //uniform LBP lookup table
    vector<int> lookup;
    int sizeb;
    //class which computes the histogram
    //of LBP features
    ocv::Histogram hist;

    /**
     * @brief countSetBits1 : method to compute the number of set bits
     * used for uniform LBP code
     * @param code
     * @return
     */
    bool countSetBits1(int code)
    {
      int count=0;
      while(code!=0)
      {
      if(code&&0x01)
      count++;
      code=code>>1;
      }
      return count;
    }

    // Brian Kernighan's method to count set bits
    int countSetBits(int code);


    int rightshift(int num, int shift);

    /**
     * @brief checkUniform : method to check if the LBP
     * pattern is uniform or not
     * @param code : input integer pattern
     * @return
     */
    bool checkUniform(int code);

    //3x3 neighborhood will have 8 neighborhood pixels
    //all non uniform codes are assigned to 59
    /**
     * @brief initUniform :inititalize the uniform LBP lookup tables
     */
    void initUniform();


    //f
    //TBD : check if grayscale encoding can be done
    //can be used to encode orientation changes of gradient
    //lbp encodes information about different types of gradients
    /**
     * @brief compute :function to compute LBP image
     * @param image : input image
     * @param dst : output LBP image
     */
    void compute(Mat image,Mat &dst);


    /**
     * @brief initHistogram : initialize the histogram object
     */
    void initHistogram();

    /**
     * @brief computeHistogram : method to compute the spatial histoogram
     * @param cell : input image ROI over which hisogram is computed
     * @return
     */
    Mat computeHistogram(Mat cell);

    /**
     * @brief spatialHistogram : computes the spatial histogram ,
     *by dividing image into grids and computing LBP for each grid
     * @param lbpImage : input LBP image
     * @param grid : grid size
     * @return  : vector for LBP histogram features
     */
    vector<float> spatialHistogram(Mat lbpImage,Size grid);

    /**
     * @brief computeBlock : divides image into blocks ,each block is assigned LBP
     * value based on mean intensity values over the block.Integral images are used for fast computation
     * @param image : input image
     * @param dst : output LBP image
     * @param block:block size
     */
    void computeBlock(Mat image,Mat & dst,int block=2);


};


class LBPTOP
{
public:
    LBPTOP()
    {
        sizeb=58;
        N=3;
        images.resize(N);
        features.resize(N);
        flag=false;
        cur_index=0;
    }

    bool flag;
    vector<Mat> images;
    int cur_index;
    int N;

    vector<vector<uchar> > features;
    Mat mask;
    IntegralImage ix;
    vector<int> lookup;
    int sizeb;

    //class which computes the histogram
    ocv::Histogram hist;

    //naive count number of set bits
    bool countSetBits1(int code)
    {
      int count=0;
      while(code!=0)
      {
      if(code&&0x01)
      count++;
      code=code>>1;
      }
      return count;
    }

    // Brian Kernighan's method to count set bits
    int countSetBits(int code)
    {
      int count=0;
      int v=code;
      for(count=0;v;count++)
      {
      v&=v-1; //clears the LSB
      }
      return count;
    }

    int rightshift(int num, int shift)
    {
        return (num >> shift) | ((num << (8 - shift)&0xFF));
    }

    //checking if code is uniform or not
    bool checkUniform(int code)
    {
        int b = rightshift(code,1);
      ///int d = code << 1;
      int c = code ^ b;
      //d= code ^d;
      int count=countSetBits(c);
      //int count1=countSetBits(d);
      if (count <=2 )
          return true;
      else
          return false;
    }

    //3x3 neighborhood will have 8 neighborhood pixels
    //all non uniform codes are assigned to 59
    void initUniform()
    {
        lookup.resize(255);
        int index=0;
        for(int i=0;i<=255;i++)
        {
            bool status=checkUniform(i);
            if(status==true)
            {
                lookup[i]=index;
                index++;
            }
            else
            {
                lookup[i]=59;
            }
        }

        initHistogram();

    }



    void initHistogram()
    {
        vector<int> channel;
        channel.push_back(0);
        hist.setChannel(channel);
        vector<int> size;
        size.push_back(sizeb+1);
        hist.setHistSize(size);

    }


    Mat computeHistogram(Mat cell)
    {
        Mat tmp_hist;
        hist.BuildHistogram(cell);
        hist.getHist().copyTo(tmp_hist);
        //tmp_hist=tmp_hist/tmp_hist.total();
        return tmp_hist;
    }

    vector<float> spatialHistogram(Mat lbpImage,Size grid)
    {
        vector<float> histogram;
        histogram.resize(grid.width*grid.height*sizeb);
        int width=lbpImage.cols/grid.width;
        int height=lbpImage.rows/grid.height;
        int cnt=0;
        //#pragma omp parallel for
        for(int i=0;i<grid.height;i++)
        {
            for(int j=0;j<grid.width;j++)
            {
                Mat cell=lbpImage(Rect(j*width,i*height,width,height));
                Mat cell_hist=computeHistogram(cell);
                //imshow("FFF",cell_hist);
                Mat tmp_feature;
                cell_hist.reshape(1,1).convertTo(tmp_feature,CV_32FC1);
                float * ptr=tmp_feature.ptr<float>(0);
                for(int k=0;k<tmp_feature.cols-1;k++)
                {
                    if(ptr[k]==0)
                        ptr[k]=1.0/sizeb;
                    histogram[(cnt*sizeb)+k]=ptr[k];
                  //  cerr << ptr[k] << endl;
                }
                break;
                cnt++;
            }
        }

        return histogram;
    }

    void compute(Mat image,Mat &dst)
    {

        cv::GaussianBlur(image,image,Size(3,3),1);
        image.copyTo(images[cur_index]);

        if(cur_index==2 && flag==false)
        {
            flag=true;
            cvtColor(image,dst,CV_GRAY2BGR);
            return;
        }

        if(flag==true)
        {


        int curr_index=(cur_index-1)%N;
        if(curr_index<0)
            curr_index=curr_index+N;

        int prev_index=(cur_index-2)%N;
        if(prev_index<0)
            prev_index=prev_index+N;


        int next_index=(cur_index)%N;

        uchar *ptr=images[curr_index].data;
        uchar *ptr1=images[prev_index].data;
        uchar *ptr2=images[next_index].data;
        //cvtColor(image,dst,CV_GRAY2BGR);
        dst.setTo(cv::Scalar::all(0));
        //image.copyTo(dst);

        uchar *optr=dst.data;

        int width=image.cols;
        int height=image.rows;

        int threshold=10;
        int cc=7;
        for(int i=1;i<height-1;i++)
        {
            for(int j=0;j<width-1;j++)
            {
                int center=(int)ptr[j+i*width];
                int center1=(int)ptr1[j+i*width];
                int center2=(int)ptr2[j+i*width];
                unsigned char code=0;
                unsigned char code1=0;
                //for(int k=0;k<3;k++)
                int k=1;
                {
                 code=0;
                 code1=0;
                //for(int k=7;k>=0;k++)
                if(k==0)
                {
                //ternanry code
                code|=((int)ptr[(j-1)+(i-1)*width] >=center+threshold)<<7;
                code|=((int)ptr[j+(i-1)*width] >=center+threshold)<<6 ;
                code|=((int)ptr[(j+1)+(i-1)*width] >=center+threshold)<<5 ;
                code|=((int)ptr[(j+1)+(i)*width] >=center+threshold)<<4 ;
                code|=((int)ptr[(j+1)+(i+1)*width] >=center+threshold)<<3 ;
                code|=((int)ptr[j+(i+1)*width] >=center+threshold)<<2 ;
                code|=((int)ptr[j-1+(i+1)*width] >=center+threshold)<<1 ;
                code|=((int)ptr[j-1+(i)*width] >=center+threshold)<<0 ;

                code1|=((int)ptr[(j-1)+(i-1)*width] <=center-threshold)<<7;
                code1|=((int)ptr[j+(i-1)*width] <=center-threshold)<<6 ;
                code1|=((int)ptr[(j+1)+(i-1)*width] <=center-threshold)<<5 ;
                code1|=((int)ptr[(j+1)+(i)*width] <=center-threshold)<<4 ;
                code1|=((int)ptr[(j+1)+(i+1)*width] <=center-threshold)<<3 ;
                code1|=((int)ptr[j+(i+1)*width] <=center-threshold)<<2 ;
                code1|=((int)ptr[j-1+(i+1)*width] <=center-threshold)<<1 ;
                code1|=((int)ptr[j-1+(i)*width] <=center-threshold)<<0 ;

                }
                else if(k==1)
                {

                    int avg=ptr1[(j-1)+(i)*width]+ptr1[(j+1)+(i)*width]+ptr1[(j)+(i-1)*width]+ptr1[(j)+(i+1)*width];
                    int avg1=ptr2[(j-1)+(i)*width]+ptr2[(j+1)+(i)*width]+ptr2[(j)+(i-1)*width]+ptr2[(j)+(i+1)*width];
                    avg=avg/4;
                    avg1=avg1/4;
                    code|=(center1 >=center+threshold)<<7;
                    code|=(center1 <=center-threshold)<<6;
                    code|=(center2 >=center+threshold)<<5;
                    code|=(center2 <=center-threshold)<<4;
                    //code|=(center2 >=center1+threshold)<=3;
                    //code|=(center2 <=center1-threshold)<=2;
                    code|=(avg >=center+threshold)<=1;
                    code|=(avg <=center-threshold)<=0;
                    code|=(avg1 >=center+threshold)<=3;
                    code|=(avg1 <=center-threshold)<=2;

                }
                else if(k==2)
                {
                    code|=((int)ptr1[(j)+(i-1)*width] >=center+threshold)<<7;
                    //code|=((int)ptr[j+(i)*width] >=center+threshold)<<6 ;
                    code|=((int)ptr2[(j)+(i+1)*width] >=center+threshold)<<5 ;
                    code|=((int)ptr2[(j+1)+(i)*width] >center+threshold )<<4 ;
                    code|=((int)ptr2[(j)+(i+1)*width] >=center+threshold)<<3 ;
                    //code|=((int)ptr[j+(i)*width] >=center+threshold)<<2 ;
                    code|=((int)ptr1[j+(i-1)*width] >=center+threshold)<<1 ;
                    code|=((int)ptr1[j+(i-1)*width] >=center+threshold)<<0 ;


                    code1|=((int)ptr1[(j)+(i-1)*width] <=center-threshold)<<7;
                    //code1|=((int)ptr[j+(i)*width]   <=center-threshold)<<6 ;
                    code1|=((int)ptr2[(j)+(i+1)*width]  <=center-threshold)<<5 ;
                    code1|=((int)ptr2[(j+1)+(i)*width] >center-threshold) <<4 ;
                    code1|=((int)ptr2[(j)+(i+1)*width]  <=center-threshold)<<3 ;

                    //code1|=((int)ptr2[j+(i)*width]  <=center-threshold)<<2 ;
                    code1|=((int)ptr1[j+(i-1)*width]  <=center-threshold)<<1 ;
                    code1|=((int)ptr1[j+(i-1)*width] >=center-threshold)<<0 ;

                }
                int check=0;
//                code1=!code1;
                //code=code|code1;
                check=code;
                //heck if the code is uniform code
                //encode only if it is a uniform code else
                //assign it a number 255
                //if(k==1)
                optr[j+(i*width)]=lookup[code];
                //else
                //optr[1*j+(1*i*width)+k]=0;

            }
        }
        }
        }


        cur_index=(cur_index+1)%N;
    }


};









#endif // LBPFEATURES_H
