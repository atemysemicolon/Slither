#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>

#include <opencv2/opencv.hpp>
#include <opencv2/ml.hpp>

#include "Sherwood.h"
#include <set>


#include <iostream>
#include <opencv2/opencv.hpp>
#include <caffe/caffe.hpp>
#include <opencv2/ximgproc.hpp>
#include <math.h>
#include <algorithm>
#include <iosfwd>
#include <memory>
#include <utility>

namespace cvml = cv::ml;

namespace MicrosoftResearch { namespace Cambridge { namespace Sherwood
{
  /// <summary>
  /// Used to describe the expected format of the lines of a data file (used
  /// in DataPointCollection::Load()).
  /// </summary>
  class DataDescriptor
  {
  public:
    enum e
    {
      Unadorned = 0x0,
      HasClassLabels = 0x1,
      HasTargetValues = 0x2
    };
  };

  /// <summary>
  /// A collection of data points, each represented by a float[] and (optionally)
  /// associated with a string class label and/or a float target value.
  /// </summary>
  class DataPointCollection: public IDataPointCollection
  {
    //std::vector<float> data_;
    cv::Mat dataMat;
    cv::Mat superpixelFeats_1;

    int dimension_;
    std::set<int> uniqueClasses_;
    bool dataPatches;
    std::vector<int> superpixel_pixel_map;
      std::vector<int> pixel_response_rows;


    std::vector<int> image_rows;
    std::vector<int> image_cols;
    long int total_size;


      // only for classified data...
    std::vector<int> labels_;

    //std::map<std::string, int> labelIndices_; // map string labels to integers

    // only for regression problems...
    std::vector<float> targets_;

  public:
    static const int UnknownClassLabel = -1;
      std::vector<std::string> filenames;


      const cv::Mat reconstructPredictions(int i, std::vector<float>& predictions) const
      {
        int start_row = 0;
        int end_row = 0;
        if(i>0)
        {
          start_row = pixel_response_rows[i-1];
        }
        end_row = pixel_response_rows[i];
        cv::Mat reconstructed = cv::Mat::zeros(image_rows[i], image_cols[i], CV_32F);
        for(int c=start_row;c<end_row;c++)
        {
          int x = (c-start_row)%reconstructed.cols;
          int y = (c-start_row)/reconstructed.cols;

//          std::cout<<" x, y : "<<x<<", "<<y<<std::endl;

          reconstructed.at<float>(y,x) = predictions[c];
        }
        //std::cout<<"Params : "<<start_row<<" "<<end_row<<" "<<reconstructed.size()<<std::endl;

        //cv::namedWindow("Annotations");
        //cv::imshow("Annotations", reconstructed*255);
        //cv::waitKey();



        return reconstructed.clone();
      }

    //FOR SCALING REQUIREMENTS - usually false
    cv::Mat scaleRow(cv::Mat& rowMat, float& bias, float& factor )
    {
      double min, max;
      cv::minMaxLoc(rowMat, &min, &max);

      if(max == 0)
        return rowMat.clone();
      bias = min;
      factor = max;
      cv::Mat m = ((rowMat-min)/max -0.5)*2;
      return m.clone();
    }

    void scaleData(std::vector<float>& biases, std::vector<float>&divisors)
    {
      cv::Mat target_mat(this->dataMat.size(),CV_32FC1);
      biases = std::vector<float> (this->dataMat.cols, 0);
      divisors = std::vector<float> (this->dataMat.cols,1);
      for(int i=0;i<this->dataMat.cols;i++)
      {
        cv::Mat colmat = this->dataMat.col(i);
        cv::Mat processedMat = scaleRow(colmat,biases[i],divisors[i]);
        processedMat.copyTo(target_mat.col(i));
      }

      this->dataMat =  target_mat.clone();
    }

    void doScaleData(const cv::Mat& biases, const cv::Mat& divisors)
    {
      cv::Mat target_mat(this->dataMat.size(),CV_32FC1);

      for(int i=0;i<this->dataMat.cols;i++)
      {
        cv::Mat colmat = this->dataMat.col(i);
        cv::Mat processedMat = (colmat-biases.at<float>(i))/ divisors.at<float>(i);
        processedMat.copyTo(target_mat.col(i));
      }

      this->dataMat =  target_mat.clone();

    }
    //END scaling part



    //For Python Stuff
    bool reserve(int H, int W)
    {
      dataMat = cv::Mat(H,W,CV_32FC1);
      dimension_ = W;
      labels_.resize(H,0);
    }

    bool putValue(float value,int label, int h,int w)
    {
      dataMat.at<float>(h,w) = value;
      labels_[h] = label;
      uniqueClasses_.insert(label);
      //std::cout<<"{";for(std::set<int>::iterator iter=uniqueClasses_.begin(); iter!=uniqueClasses_.end();++iter) { std::cout<<(*iter);}std::cout<<"}"<<std::endl;


        //std::cout<<"Putting Value : "<<value<<" lbl : "<<label<<std::endl;
    }
    //ENd PYTHON REQUIREMENTS

    /// <summary>
    /// Load a collection of data from a tab-delimited file with one data point
    /// per line. The data may optionally have associated with class labels
    /// (first element on line) and/or target values (last element on line).
    /// </summary>
    /// <param name="path">Path of file to be read.</param>
    /// <param name="bHasClassLabels">Are the data associated with class labels?</param>
    /// <param name="dataDimension">Dimension of the data (excluding class labels and target values).</param>
    /// <param name="bHasTargetValues">Are the data associated with target values.</param>
    //static  std::auto_ptr<DataPointCollection> Load(std::istream& r, int dataDimension, DataDescriptor::e descriptor);

     static  std::auto_ptr<DataPointCollection> Load(const std::string &filename);
     static  std::auto_ptr<DataPointCollection> LoadPatches(const std::string &filename, const std::string &img_folder, const std::string &ann_folder);

    /// <summary>
    /// Generate a 2D dataset with data points distributed in a grid pattern.
    /// Intended for generating visualization images.
    /// </summary>
    /// <param name="rangeX">x-axis range</param>
    /// <param name="nStepsX">Number of grid points in x direction</param>
    /// <param name="rangeY">y-axis range</param>
    /// <param name="nStepsY">Number of grid points in y direction</param>
    /// <returns>A new DataPointCollection</returns>
    static  std::auto_ptr<DataPointCollection> Generate2dGrid(
      std::pair<float, float> rangeX, int nStepsX,
      std::pair<float, float> rangeY, int nStepsY);

    /// <summary>
    /// Generate a 1D dataset containing a given number of data points
    /// distributed at regular intervals within a given range. Intended for
    /// generating visualization images.
    /// </summary>
    /// <param name="range">Range</param>
    /// <param name="nStepsX">Number of grid points</param>
    /// <returns>A new DataPointCollection</returns>
    static std::auto_ptr<DataPointCollection> Generate1dGrid(std::pair<float, float> range, int nSteps);

    /// <summary>
    /// Do these data have class labels?
    /// </summary>
    bool HasLabels() const
    {
      return labels_.size() != 0;
    }

    /// <summary>
    /// How many unique class labels are there?
    /// </summary>
    /// <returns>The number of unique class labels</returns>
    int CountClasses() const
    {
      if (!HasLabels())
        throw std::runtime_error("Unlabelled data.");

      //return labelIndices_.size();
      return uniqueClasses_.size();
    }

    /// <summary>
    /// Do these data have target values (e.g. for regression)?
    /// </summary>
    bool HasTargetValues() const
    {
      return targets_.size() != 0;
    }

    /// <summary>
    /// Count the data points in this collection.
    /// </summary>
    /// <returns>The number of data points</returns>
    unsigned int Count() const
    {
      return dataMat.rows;
    }

    void showMat() const
    {
      std::cout<<dataMat<<std::endl;
      std::cout<<dataMat.size()<<std::endl;
    }

    /// <summary>
    /// Get the data range in the specified data dimension.
    /// </summary>
    /// <param name="dimension">The dimension over which to compute min and max</param>
    /// <returns>A tuple containing min and max over the specified dimension of the data</returns>
    std::pair<float, float> GetRange(int dimension) const;

    /// <summary>
    /// Get the range of target values (or raise an exception if these data
    /// do not have associated target values).
    /// </summary>
    /// <returns>A tuple containing the min and max target value for the data</returns>
    std::pair<float, float> GetTargetRange() const;

    /// <summary>
    /// The dimensionality of the data (excluding optional target values).
    /// </summary>
    int Dimensions() const
    {
      return dimension_;
    }

    /// <summary>
    /// Get the specified data point.
    /// </summary>
    /// <param name="i">Zero-based data point index.</param>
    /// <returns>Pointer to the first element of the data point.</returns>
    /*const float* GetDataPoint(int i) const
    {
      return &data_[i*dimension_];
    }*/

    /// <summary>
    /// Get the specified data point.
    /// </summary>
    /// <param name="i">Zero-based data point index.</param>
    /// <returns>Row values of the first element of the data point.</returns>
    const cv::Mat GetDataPoint(int i) const
    {
      return dataMat.row(i);
    }

    const cv::Mat GetDataPoint(int i, bool superpixel_choice) const
    {

      if(!superpixel_choice)
        return dataMat.row(i);
      else
        return superpixelFeats_1.row(superpixel_pixel_map[i]);
    }

    const cv::Mat reconstructAnn(int i)
    {
      int start_row = 0;
      int end_row = 0;
      int start_col = 0;
      int end_col=0;
      if(i!=0)
      {
        start_row = image_rows[i-1];
        start_col  =image_cols[i-1];
      }

      end_row = image_rows[i];
      end_col = image_cols[i];
      int c = 0;

      cv::Mat reconstructed = cv::Mat::zeros(end_row-start_row, end_col-start_col, CV_32F);
      for(int y=0;y<reconstructed.rows;y++)
        for(int x=0;x<reconstructed.cols;x++)
        {
          reconstructed.at<float>(y,x) = labels_[start_row+(c++)];
        }

      return reconstructed;
    }





    cv::Ptr<cvml::TrainData> getTrainData()
    {

      return cvml::TrainData::create(dataMat, cvml::ROW_SAMPLE, cv::Mat(labels_));

    }

    cv::Ptr<cvml::TrainData> getTrainDataWithMask(std::vector<int> mask_values, int start_row, int end_row)
    {
      cv::Mat colMat = dataMat.rowRange(start_row,end_row);
      //std::cout<<"--->"<<dataMat.rows<<std::endl;
      cv::Mat labelsMat = cv::Mat(labels_);
      cv::Mat reducedLabels = labelsMat.rowRange(start_row,end_row);
      return cvml::TrainData::create(colMat, cvml::ROW_SAMPLE, reducedLabels, mask_values);
    }


    cv::Ptr<cvml::TrainData> getTrainDataWithMaskOrdered(std::vector<int> mask_values, int start_row, int end_row, unsigned int* indices)
    {
      cv::Mat colMat;
      //std::cout<<"--->"<<dataMat.rows<<std::endl;
      cv::Mat labelsMat = cv::Mat(labels_);
      cv::Mat reducedLabels;
      for(int i=start_row;i<end_row;i++)
      {
        int j = indices[i];
        colMat.push_back(dataMat.row(j));
        reducedLabels.push_back(labelsMat.row(j));
      }

      //std::cout<<colMat.size()<<std::endl;
      //std::cout<<reducedLabels.size()<<std::endl;

      return cvml::TrainData::create(colMat, cvml::ROW_SAMPLE, reducedLabels, mask_values);
    }


    cv::Ptr<cvml::TrainData> getTrainDataWithMaskOrderedSuperpixel(std::vector<int> mask_values, int start_row, int end_row, unsigned int* indices)
    {
      cv::Mat colMat;
      //std::cout<<"--->"<<dataMat.rows<<std::endl;
      cv::Mat labelsMat = cv::Mat(labels_);
      cv::Mat reducedLabels;
      for(int i=start_row;i<end_row;i++)
      {
        int j = indices[i];
        colMat.push_back(superpixelFeats_1.row(superpixel_pixel_map[j]));
        reducedLabels.push_back(labelsMat.row(j));
      }

      //std::cout<<colMat.size()<<std::endl;
      //std::cout<<reducedLabels.size()<<std::endl;

      return cvml::TrainData::create(colMat, cvml::ROW_SAMPLE, reducedLabels, mask_values);
    }






    /// <summary>
    /// Get the class label for the specified data point (or raise an
    /// exception if these data points do not have associated labels).
    /// </summary>
    /// <param name="i">Zero-based data point index</param>
    /// <returns>A zero-based integer class label.</returns>
    int GetIntegerLabel(int i) const
    {
      if (!HasLabels())
        throw std::runtime_error("Data have no associated class labels.");
      return labels_[i]; // may throw an exception if index is out of range
    }



    /// <summary>
    /// Get the target value for the specified data point (or raise an
    /// exception if these data points do not have associated target values).
    /// </summary>
    /// <param name="i">Zero-based data point index.</param>
    /// <returns>The target value.</returns>
    float GetTarget(int i) const
    {
      if (!HasTargetValues())
        throw std::runtime_error("Data have no associated target values.");

      return targets_[i]; // may throw an exception if index is out of range
    }
  };

  // A couple of file parsing utilities, exposed here for testing only.

  // Split a delimited line into constituent elements.
  void tokenize(
    const std::string& str,
    std::vector<std::string>& tokens,
    const std::string& delimiters = " " );

  // Convert a std::string to a float (or raise an exception).
  float to_float(const std::string& s);



  cv::Mat to_index_kitti(const cv::Mat &img);


  //Classifier
  using namespace caffe;  // NOLINT(build/namespaces)
  using std::string;
  typedef std::pair<string, float> Prediction;

  class Classifier {
  public:
      cv::Size input_geometry_;

      Classifier(const string& model_file,
                 const string& trained_file,
                 const string& mean_file,
                 const string& label_file);

      std::vector<Prediction> Classify(const cv::Mat& img, int N = 5);
      void forwardPass(const cv::Mat &img);
      cv::Mat forwardPass(const cv::Mat &img,int layer_nr);
      std::vector<cv::Mat> forwardPassVector(const cv::Mat &img,int layer_nr);

  private:
      //void SetMean(const string& mean_file);

      std::vector<float> Predict(const cv::Mat& img);

      void WrapInputLayer(std::vector<cv::Mat>* input_channels);

      void Preprocess(const cv::Mat& img,
                      std::vector<cv::Mat>* input_channels);

  private:
      std::shared_ptr<Net<float> > net_;
      int num_channels_;
      cv::Mat mean_;
      std::vector<string> labels_;
  };




} } }
