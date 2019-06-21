/* @file   stereoVIOKitti.cpp
 * @brief  example of VIO pipeline running on the KITTI dataset
 * @author Yun Chang based off stereoVIOEuroc.cpp
 */

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "ETH_parser.h"
#include "datasource/KittiDataSource.h"
#include "pipeline/Pipeline.h"
#include "utils/Timer.h"
#include "LoggerMatlab.h"

#include <future>

DEFINE_bool(parallel_run, false, "Run parallelized pipeline.");
// clean up later (dataset_path definition in ETH_parser.cpp)
DEFINE_string(kitti_dataset_path, "/home/yunchang/data/2011_09_26/2011_09_26_drive_0113_sync",
    "Path of dataset (i.e. Kitti, /home/yunchang/data/2011_09_26/2011_09_26_drive_0113_sync");

////////////////////////////////////////////////////////////////////////////////
// stereoVIOexample
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
  // Initialize Google's flags library.
  google::ParseCommandLineFlags(&argc, &argv, true);
  // Initialize Google's logging library.
  google::InitGoogleLogging(argv[0]);

  // Ctor ETHDatasetParser, and parse dataset.
  VIO::ETHDatasetParser eth_dataset_parser;
  VIO::KittiDataProvider kitti_dataset_parser(FLAGS_kitti_dataset_path);

  VIO::Pipeline vio_pipeline (&eth_dataset_parser, kitti_dataset_parser.getImuParams()); 

  // Register callback to vio_pipeline.
  kitti_dataset_parser.registerVioCallback(
        std::bind(&VIO::Pipeline::spin, &vio_pipeline, std::placeholders::_1));

  // Spin dataset.
  auto tic = VIO::utils::Timer::tic();
  auto handle = std::async(std::launch::async,
                           &VIO::KittiDataProvider::spin, &kitti_dataset_parser);
  auto handle_pipeline = std::async(std::launch::async,
             &VIO::Pipeline::shutdownWhenFinished, &vio_pipeline);
  vio_pipeline.spinViz();
  const bool is_pipeline_successful = handle.get();
  handle_pipeline.get();

  auto spin_duration = VIO::utils::Timer::toc(tic);
  LOG(WARNING) << "Spin took: " << spin_duration.count() << " ms.";

  if (is_pipeline_successful) {
    // Log overall time of pipeline run.
    VIO::LoggerMatlab logger;
    logger.openLogFiles(11);
    logger.logPipelineOverallTiming(spin_duration);
    logger.closeLogFiles();
  }

  return is_pipeline_successful? EXIT_SUCCESS : EXIT_FAILURE;
}
