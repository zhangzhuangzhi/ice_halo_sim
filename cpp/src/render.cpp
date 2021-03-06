#include "render.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

#include "context.h"
#include "mymath.h"
#include "optics.h"
#include "threadingpool.h"


namespace IceHalo {

void EqualAreaFishEye(const float* cam_rot,          // Camera rotation. [lon, lat, roll]
                      float hov,                     // Half field of view.
                      size_t data_number,            // Data number
                      const float* dir,              // Ray directions, [x, y, z]
                      int img_wid, int img_hei,      // Image size
                      int* img_xy,                   // Image coordinates
                      VisibleRange visible_range) {  // Visible range
  float img_r = std::max(img_wid, img_hei) / 2.0f;
  auto* dir_copy = new float[data_number * 3];
  float cam_rot_copy[3];
  std::memcpy(cam_rot_copy, cam_rot, sizeof(float) * 3);
  cam_rot_copy[0] *= -1;
  cam_rot_copy[1] *= -1;
  for (auto& i : cam_rot_copy) {
    i *= Math::kDegreeToRad;
  }

  Math::RotateZWithDataStep(cam_rot_copy, dir, dir_copy, 4, 3, data_number);
  for (decltype(data_number) i = 0; i < data_number; i++) {
    if (std::abs(Math::Norm3(dir_copy + i * 3) - 1.0) > 1e-4) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else if (visible_range == VisibleRange::kFront && dir_copy[i * 3 + 2] < 0) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else if (visible_range == VisibleRange::kUpper && dir[i * 4 + 2] > 0) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else if (visible_range == VisibleRange::kLower && dir[i * 4 + 2] < 0) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else {
      float lon = std::atan2(dir_copy[i * 3 + 1], dir_copy[i * 3 + 0]);
      float lat = std::asin(dir_copy[i * 3 + 2] / Math::Norm3(dir_copy + i * 3));
      float proj_r = img_r / 2.0f / std::sin(hov / 2.0f * Math::kDegreeToRad);
      float r = 2.0f * proj_r * std::sin((Math::kPi / 2.0f - lat) / 2.0f);

      img_xy[i * 2 + 0] = static_cast<int>(std::round(r * std::cos(lon) + img_wid / 2.0f));
      img_xy[i * 2 + 1] = static_cast<int>(std::round(r * std::sin(lon) + img_hei / 2.0f));
    }
  }

  delete[] dir_copy;
}


void DualEqualAreaFishEye(const float* /* cam_rot */,          // Not used
                          float /* hov */,                     // Not used
                          size_t data_number,                  // Data number
                          const float* dir,                    // Ray directions, [x, y, z]
                          int img_wid, int img_hei,            // Image size
                          int* img_xy,                         // Image coordinates
                          VisibleRange /* visible_range */) {  // Not used
  float img_r = std::min(img_wid / 2, img_hei) / 2.0f;
  float proj_r = img_r / 2.0f / std::sin(45.0f * Math::kDegreeToRad);

  auto* dir_copy = new float[data_number * 3];
  float cam_rot_copy[3] = { 90.0f, 89.999f, 0.0f };
  cam_rot_copy[0] *= -1;
  cam_rot_copy[1] *= -1;
  for (auto& i : cam_rot_copy) {
    i *= Math::kDegreeToRad;
  }

  Math::RotateZWithDataStep(cam_rot_copy, dir, dir_copy, 4, 3, data_number);
  for (decltype(data_number) i = 0; i < data_number; i++) {
    if (std::abs(Math::Norm3(dir_copy + i * 3) - 1.0) > 1e-4) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else {
      float lon = std::atan2(dir_copy[i * 3 + 1], dir_copy[i * 3 + 0]);
      float lat = std::asin(dir_copy[i * 3 + 2] / Math::Norm3(dir_copy + i * 3));
      if (lat < 0) {
        lon = Math::kPi - lon;
      }
      float r = 2.0f * proj_r * std::sin((Math::kPi / 2.0f - std::abs(lat)) / 2.0f);

      img_xy[i * 2 + 0] = static_cast<int>(std::round(r * std::cos(lon) + img_r + (lat > 0 ? -0.5 : 2 * img_r - 0.5)));
      img_xy[i * 2 + 1] = static_cast<int>(std::round(r * std::sin(lon) + img_r - 0.5));
    }
  }

  delete[] dir_copy;
}


void DualEquidistantFishEye(const float* /* cam_rot */,          // Not used
                            float /* hov */,                     // Not used
                            size_t data_number,                  // Data number
                            const float* dir,                    // Ray directions, [x, y, z]
                            int img_wid, int img_hei,            // Image size
                            int* img_xy,                         // Image coordinates
                            VisibleRange /* visible_range */) {  // Not used
  float img_r = std::min(img_wid / 2, img_hei) / 2.0f;

  auto* dir_copy = new float[data_number * 3];
  float cam_rot_copy[3] = { 90.0f, 89.999f, 0.0f };
  cam_rot_copy[0] *= -1;
  cam_rot_copy[1] *= -1;
  for (auto& i : cam_rot_copy) {
    i *= Math::kDegreeToRad;
  }

  Math::RotateZWithDataStep(cam_rot_copy, dir, dir_copy, 4, 3, data_number);
  for (decltype(data_number) i = 0; i < data_number; i++) {
    if (std::abs(Math::Norm3(dir_copy + i * 3) - 1.0) > 1e-4) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else {
      float lon = std::atan2(dir_copy[i * 3 + 1], dir_copy[i * 3 + 0]);
      float lat = std::asin(dir_copy[i * 3 + 2] / Math::Norm3(dir_copy + i * 3));
      if (lat < 0) {
        lon = Math::kPi - lon;
      }
      float r = (1.0f - std::abs(lat) * 2.0f / Math::kPi) * img_r;

      img_xy[i * 2 + 0] = static_cast<int>(std::round(r * std::cos(lon) + img_r + (lat > 0 ? -0.5 : 2 * img_r - 0.5)));
      img_xy[i * 2 + 1] = static_cast<int>(std::round(r * std::sin(lon) + img_r - 0.5));
    }
  }

  delete[] dir_copy;
}


void RectLinear(const float* cam_rot,          // Camera rotation. [lon, lat, roll]
                float hov,                     // Half field of view.
                size_t data_number,            // Data number
                const float* dir,              // Ray directions, [x, y, z]
                int img_wid, int img_hei,      // Image size
                int* img_xy,                   // Image coordinates
                VisibleRange visible_range) {  // Visible range
  auto* dir_copy = new float[data_number * 3];
  float cam_rot_copy[3];
  std::memcpy(cam_rot_copy, cam_rot, sizeof(float) * 3);
  cam_rot_copy[0] *= -1;
  cam_rot_copy[1] *= -1;
  for (auto& i : cam_rot_copy) {
    i *= Math::kDegreeToRad;
  }

  Math::RotateZWithDataStep(cam_rot_copy, dir, dir_copy, 4, 3, data_number);
  for (decltype(data_number) i = 0; i < data_number; i++) {
    if (dir_copy[i * 3 + 2] < 0 || std::abs(Math::Norm3(dir_copy + i * 3) - 1.0) > 1e-4) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else if (visible_range == VisibleRange::kFront && dir_copy[i * 3 + 2] < 0) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else if (visible_range == VisibleRange::kUpper && dir[i * 4 + 2] > 0) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else if (visible_range == VisibleRange::kLower && dir[i * 4 + 2] < 0) {
      img_xy[i * 2 + 0] = std::numeric_limits<int>::min();
      img_xy[i * 2 + 1] = std::numeric_limits<int>::min();
    } else {
      float x = dir_copy[i * 3 + 0] / dir_copy[i * 3 + 2];
      float y = dir_copy[i * 3 + 1] / dir_copy[i * 3 + 2];
      x = x * img_wid / 2 / std::tan(hov * Math::kDegreeToRad) + img_wid / 2.0f;
      y = y * img_wid / 2 / std::tan(hov * Math::kDegreeToRad) + img_hei / 2.0f;

      img_xy[i * 2 + 0] = static_cast<int>(std::round(x));
      img_xy[i * 2 + 1] = static_cast<int>(std::round(y));
    }
  }

  delete[] dir_copy;
}


MyUnorderedMap<LensType, ProjectionFunction>& GetProjectionFunctions() {
  static MyUnorderedMap<LensType, ProjectionFunction> projection_functions = {
    { LensType::kLinear, &RectLinear },
    { LensType::kEqualArea, &EqualAreaFishEye },
    { LensType::kDualEquidistant, &DualEquidistantFishEye },
    { LensType::kDualEqualArea, &DualEqualAreaFishEye },
  };

  return projection_functions;
}


void SrgbGamma(float* linear_rgb) {
  for (int i = 0; i < 3; i++) {
    if (linear_rgb[i] < 0.0031308) {
      linear_rgb[i] *= 12.92f;
    } else {
      linear_rgb[i] = static_cast<float>(1.055 * std::pow(linear_rgb[i], 1.0 / 2.4) - 0.055);
    }
  }
}


constexpr int SpectrumRenderer::kMinWavelength;
constexpr int SpectrumRenderer::kMaxWaveLength;
constexpr uint8_t SpectrumRenderer::kColorMaxVal;
constexpr float SpectrumRenderer::kWhitePointD65[];
constexpr float SpectrumRenderer::kXyzToRgb[];
constexpr float SpectrumRenderer::kCmfX[];
constexpr float SpectrumRenderer::kCmfY[];
constexpr float SpectrumRenderer::kCmfZ[];


SpectrumRenderer::SpectrumRenderer(ProjectContextPtr context) : context_(std::move(context)), total_w_(0) {}


SpectrumRenderer::~SpectrumRenderer() {
  ResetData();
}


void SpectrumRenderer::LoadData() {
  auto projection_type = context_->cam_ctx_.GetLensType();
  const auto& projection_functions = GetProjectionFunctions();
  if (projection_functions.find(projection_type) == projection_functions.end()) {
    std::fprintf(stderr, "Unknown projection type!\n");
    return;
  }

  std::vector<File> files = ListDataFiles(context_->GetDataDirectory().c_str());
  int i = 0;
  for (auto& f : files) {
    auto t0 = std::chrono::system_clock::now();
    auto num = LoadDataFromFile(f);
    auto t1 = std::chrono::system_clock::now();
    std::chrono::duration<float, std::ratio<1, 1000>> diff = t1 - t0;
    std::printf(" Loading data (%d/%zu): %.2fms; total %d pts\n", i + 1, files.size(), diff.count(), num);
    i++;
  }
}


void SpectrumRenderer::LoadData(float wl, float weight, const float* ray_data, size_t num) {
  auto projection_type = context_->cam_ctx_.GetLensType();
  auto& projection_functions = GetProjectionFunctions();
  if (projection_functions.find(projection_type) == projection_functions.end()) {
    std::fprintf(stderr, "Unknown projection type!\n");
    return;
  }
  auto& pf = projection_functions[projection_type];

  auto wavelength = static_cast<int>(wl);
  if (wavelength < SpectrumRenderer::kMinWavelength || wavelength > SpectrumRenderer::kMaxWaveLength || weight < 0) {
    std::fprintf(stderr, "Wavelength out of range!\n");
    return;
  }

  auto* tmp_xy = new int[num * 2];

  auto img_hei = context_->render_ctx_.GetImageHeight();
  auto img_wid = context_->render_ctx_.GetImageWidth();

  auto threading_pool = ThreadingPool::GetInstance();
  auto step = std::max(num / 100, static_cast<size_t>(10));
  for (decltype(num) i = 0; i < num; i += step) {
    decltype(num) current_num = std::min(num - i, step);
    threading_pool->AddJob([=] {
      pf(context_->cam_ctx_.GetCameraTargetDirection(), context_->cam_ctx_.GetFov(), current_num, ray_data + i * 4,
         img_wid, img_hei, tmp_xy + i * 2, context_->render_ctx_.GetVisibleRange());
    });
  }
  threading_pool->WaitFinish();

  float* current_data = nullptr;
  float* current_data_compensation = nullptr;
  auto it = spectrum_data_.find(wavelength);
  if (it != spectrum_data_.end()) {
    current_data = it->second;
    current_data_compensation = spectrum_data_compensation_[wavelength];
  } else {
    current_data = new float[img_hei * img_wid];
    current_data_compensation = new float[img_hei * img_wid];
    for (decltype(img_hei) i = 0; i < img_hei * img_wid; i++) {
      current_data[i] = 0;
      current_data_compensation[i] = 0;
    }
    spectrum_data_[wavelength] = current_data;
    spectrum_data_compensation_[wavelength] = current_data_compensation;
  }

  for (decltype(num) i = 0; i < num; i++) {
    int x = tmp_xy[i * 2 + 0];
    int y = tmp_xy[i * 2 + 1];
    if (x == std::numeric_limits<int>::min() || y == std::numeric_limits<int>::min()) {
      continue;
    }
    if (projection_type != LensType::kDualEqualArea && projection_type != LensType::kDualEquidistant) {
      x += context_->render_ctx_.GetImageOffsetX();
      y += context_->render_ctx_.GetImageOffsetY();
    }
    if (x < 0 || x >= static_cast<int>(img_wid) || y < 0 || y >= static_cast<int>(img_hei)) {
      continue;
    }
    auto tmp_val = ray_data[i * 4 + 3] * weight - current_data_compensation[y * img_wid + x];
    auto tmp_sum = current_data[y * img_wid + x] + tmp_val;
    current_data_compensation[y * img_wid + x] = tmp_sum - current_data[y * img_wid + x] - tmp_val;
    current_data[y * img_wid + x] = tmp_sum;
  }
  delete[] tmp_xy;

  total_w_ += context_->GetInitRayNum() * weight;
}


void SpectrumRenderer::ResetData() {
  total_w_ = 0;
  for (const auto& kv : spectrum_data_) {
    delete[] kv.second;
  }
  for (const auto& kv : spectrum_data_compensation_) {
    delete[] kv.second;
  }
  spectrum_data_.clear();
  spectrum_data_compensation_.clear();
}


void SpectrumRenderer::RenderToRgb(uint8_t* rgb_data) {
  auto img_hei = context_->render_ctx_.GetImageHeight();
  auto img_wid = context_->render_ctx_.GetImageWidth();
  auto wl_num = spectrum_data_.size();
  auto* wl_data = new float[wl_num];
  auto* flat_spec_data = new float[wl_num * img_wid * img_hei];

  GatherSpectrumData(wl_data, flat_spec_data);
  auto ray_color = context_->render_ctx_.GetRayColor();
  auto background_color = context_->render_ctx_.GetBackgroundColor();
  bool use_rgb = ray_color[0] < 0;

  if (use_rgb) {
    Rgb(wl_num, img_wid * img_hei, wl_data, flat_spec_data, rgb_data);
  } else {
    Gray(wl_num, img_wid * img_hei, wl_data, flat_spec_data, rgb_data);
  }
  for (decltype(img_wid) i = 0; i < img_wid * img_hei; i++) {
    for (int c = 0; c < 3; c++) {
      auto v = static_cast<int>(background_color[c] * kColorMaxVal);
      if (use_rgb) {
        v += rgb_data[i * 3 + c];
      } else {
        v += static_cast<int>(rgb_data[i * 3 + c] * ray_color[c]);
      }
      v = std::max(std::min(v, static_cast<int>(kColorMaxVal)), 0);
      rgb_data[i * 3 + c] = static_cast<uint8_t>(v);
    }
  }

  /* Draw horizontal */
  // float imgR = std::min(img_wid_ / 2, img_hei_) / 2.0f;
  // TODO

  delete[] wl_data;
  delete[] flat_spec_data;
}


int SpectrumRenderer::LoadDataFromFile(IceHalo::File& file) {
  auto file_size = file.GetSize();
  auto* read_buffer = new float[file_size / sizeof(float)];

  file.Open(OpenMode::kRead | OpenMode::kBinary);
  auto read_count = file.Read(read_buffer, 2);
  if (read_count <= 0) {
    std::fprintf(stderr, "Failed to read wavelength data!\n");
    file.Close();
    delete[] read_buffer;
    return -1;
  }

  auto wavelength = static_cast<int>(read_buffer[0]);
  auto wavelength_weight = read_buffer[1];
  if (wavelength < SpectrumRenderer::kMinWavelength || wavelength > SpectrumRenderer::kMaxWaveLength ||
      wavelength_weight < 0) {
    std::fprintf(stderr, "Wavelength out of range!\n");
    file.Close();
    delete[] read_buffer;
    return -1;
  }

  read_count = file.Read(read_buffer, file_size / sizeof(float));
  auto total_ray_count = read_count / 4;
  file.Close();

  if (total_ray_count == 0) {
    delete[] read_buffer;
    return 0;
  }

  LoadData(wavelength, wavelength_weight, read_buffer, total_ray_count);
  delete[] read_buffer;

  return static_cast<int>(total_ray_count);
}


void SpectrumRenderer::GatherSpectrumData(float* wl_data_out, float* sp_data_out) {
  auto img_hei = context_->render_ctx_.GetImageHeight();
  auto img_wid = context_->render_ctx_.GetImageWidth();
  auto intensity_factor = static_cast<float>(context_->render_ctx_.GetIntensity());

  int k = 0;
  for (const auto& kv : spectrum_data_) {
    wl_data_out[k] = kv.first;
    std::memcpy(sp_data_out + k * img_wid * img_hei, kv.second, img_wid * img_hei * sizeof(float));
    k++;
  }

  auto factor = 1e5f / total_w_ * intensity_factor;
  for (decltype(spectrum_data_.size()) i = 0; i < img_wid * img_hei * spectrum_data_.size(); i++) {
    sp_data_out[i] *= factor;
  }
}


void SpectrumRenderer::Rgb(size_t wavelength_number, size_t data_number,  //
                           const float* wavelengths,
                           const float* spec_data,  // spec_data: wavelength_number x data_number
                           uint8_t* rgb_data) {     // rgb data, data_number x 3
  for (decltype(data_number) i = 0; i < data_number; i++) {
    /* Step 1. Spectrum to XYZ */
    float xyz[3] = { 0 };
    for (decltype(wavelength_number) j = 0; j < wavelength_number; j++) {
      auto wl = static_cast<int>(wavelengths[j]);
      if (wl < kMinWavelength || wl > kMaxWaveLength) {
        continue;
      }
      float v = spec_data[j * data_number + i];
      xyz[0] += kCmfX[wl - kMinWavelength] * v;
      xyz[1] += kCmfY[wl - kMinWavelength] * v;
      xyz[2] += kCmfZ[wl - kMinWavelength] * v;
    }

    /* Step 2. XYZ to linear RGB */
    float gray[3];
    for (int j = 0; j < 3; j++) {
      gray[j] = kWhitePointD65[j] * xyz[1];
    }

    float r = 1.0f;
    for (int j = 0; j < 3; j++) {
      float a = 0, b = 0;
      for (int k = 0; k < 3; k++) {
        a += -gray[k] * kXyzToRgb[j * 3 + k];
        b += (xyz[k] - gray[k]) * kXyzToRgb[j * 3 + k];
      }
      if (a * b > 0 && a / b < r) {
        r = a / b;
      }
    }

    float rgb[3] = { 0 };
    for (int j = 0; j < 3; j++) {
      xyz[j] = (xyz[j] - gray[j]) * r + gray[j];
    }
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        rgb[j] += xyz[k] * kXyzToRgb[j * 3 + k];
      }
      rgb[j] = std::min(std::max(rgb[j], 0.0f), 1.0f);
    }

    /* Step 3. Convert linear sRGB to sRGB */
    SrgbGamma(rgb);
    for (int j = 0; j < 3; j++) {
      rgb_data[i * 3 + j] = static_cast<uint8_t>(rgb[j] * 255);
    }
  }
}


void SpectrumRenderer::Gray(size_t wavelength_number, size_t data_number,  //
                            const float* wavelengths,
                            const float* spec_data,  // spec_data: wavelength_number x data_number
                            uint8_t* rgb_data) {     // rgb data, data_number x 3
  for (decltype(data_number) i = 0; i < data_number; i++) {
    /* Step 1. Spectrum to XYZ */
    float xyz[3] = { 0 };
    for (decltype(wavelength_number) j = 0; j < wavelength_number; j++) {
      auto wl = static_cast<int>(wavelengths[j]);
      if (wl < kMinWavelength || wl > kMaxWaveLength) {
        continue;
      }
      float v = spec_data[j * data_number + i];
      xyz[0] += kCmfX[wl - kMinWavelength] * v;
      xyz[1] += kCmfY[wl - kMinWavelength] * v;
      xyz[2] += kCmfZ[wl - kMinWavelength] * v;
    }

    /* Step 2. XYZ to linear RGB */
    float gray[3];
    for (int j = 0; j < 3; j++) {
      gray[j] = kWhitePointD65[j] * xyz[1];
    }

    float rgb[3] = { 0 };
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        rgb[j] += gray[k] * kXyzToRgb[j * 3 + k];
      }
      rgb[j] = std::min(std::max(rgb[j], 0.0f), 1.0f);
    }

    /* Step 3. Convert linear sRGB to sRGB */
    SrgbGamma(rgb);
    for (int j = 0; j < 3; j++) {
      rgb_data[i * 3 + j] = static_cast<uint8_t>(rgb[j] * 255);
    }
  }
}

}  // namespace IceHalo
