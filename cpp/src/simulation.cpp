#include "simulation.h"

#include <cstdio>
#include <stack>
#include <utility>

#include "mymath.h"
#include "threadingpool.h"

namespace IceHalo {

SimulationBufferData::SimulationBufferData()
    : pt{ nullptr }, dir{ nullptr }, w{ nullptr }, face_id{ nullptr }, ray_seg{ nullptr }, ray_num(0) {}


SimulationBufferData::~SimulationBufferData() {
  Clean();
}


void SimulationBufferData::Clean() {
  for (int i = 0; i < 2; i++) {
    DeleteBuffer(i);
  }
  ray_num = 0;
}


void SimulationBufferData::DeleteBuffer(int idx) {
  delete[] pt[idx];
  delete[] dir[idx];
  delete[] w[idx];
  delete[] face_id[idx];
  delete[] ray_seg[idx];

  pt[idx] = nullptr;
  dir[idx] = nullptr;
  w[idx] = nullptr;
  face_id[idx] = nullptr;
  ray_seg[idx] = nullptr;
}


void SimulationBufferData::Allocate(size_t ray_number) {
  for (int i = 0; i < 2; i++) {
    auto tmp_pt = new float[ray_number * 3];
    auto tmp_dir = new float[ray_number * 3];
    auto tmp_w = new float[ray_number];
    auto tmp_face_id = new int[ray_number];
    auto tmp_ray_seg = new RaySegment*[ray_number];

    if (pt[i]) {
      size_t n = std::min(this->ray_num, ray_number);
      std::memcpy(tmp_pt, pt[i], sizeof(float) * 3 * n);
      std::memcpy(tmp_dir, dir[i], sizeof(float) * 3 * n);
      std::memcpy(tmp_w, w[i], sizeof(float) * n);
      std::memcpy(tmp_face_id, face_id[i], sizeof(int) * n);
      std::memcpy(tmp_ray_seg, ray_seg[i], sizeof(void*) * n);

      DeleteBuffer(i);
    }

    pt[i] = tmp_pt;
    dir[i] = tmp_dir;
    w[i] = tmp_w;
    face_id[i] = tmp_face_id;
    ray_seg[i] = tmp_ray_seg;
  }
  this->ray_num = ray_number;
}


void SimulationBufferData::Print() {
  std::printf("pt[0]                    dir[0]                   w[0]\n");
  for (decltype(ray_num) i = 0; i < ray_num; i++) {
    std::printf("%+.4f,%+.4f,%+.4f  ", pt[0][i * 3 + 0], pt[0][i * 3 + 1], pt[0][i * 3 + 2]);
    std::printf("%+.4f,%+.4f,%+.4f  ", dir[0][i * 3 + 0], dir[0][i * 3 + 1], dir[0][i * 3 + 2]);
    std::printf("%+.4f\n", w[0][i]);
  }

  std::printf("pt[1]                    dir[1]                   w[1]\n");
  for (decltype(ray_num) i = 0; i < ray_num; i++) {
    std::printf("%+.4f,%+.4f,%+.4f  ", pt[1][i * 3 + 0], pt[1][i * 3 + 1], pt[1][i * 3 + 2]);
    std::printf("%+.4f,%+.4f,%+.4f  ", dir[1][i * 3 + 0], dir[1][i * 3 + 1], dir[1][i * 3 + 2]);
    std::printf("%+.4f\n", w[1][i]);
  }
}


EnterRayData::EnterRayData() : ray_dir(nullptr), ray_seg(nullptr), ray_num(0) {}


EnterRayData::~EnterRayData() {
  DeleteBuffer();
}


void EnterRayData::Clean() {
  DeleteBuffer();
  ray_num = 0;
}


void EnterRayData::Allocate(size_t ray_number) {
  DeleteBuffer();

  ray_dir = new float[ray_number * 3];
  ray_seg = new RaySegment*[ray_number];

  for (decltype(ray_number) i = 0; i < ray_number; i++) {
    ray_dir[i * 3 + 0] = 0;
    ray_dir[i * 3 + 1] = 0;
    ray_dir[i * 3 + 2] = 0;
    ray_seg[i] = nullptr;
  }

  this->ray_num = ray_number;
}


void EnterRayData::DeleteBuffer() {
  delete[] ray_dir;
  delete[] ray_seg;

  ray_dir = nullptr;
  ray_seg = nullptr;
}


Simulator::Simulator(ProjectContextPtr context)
    : context_(std::move(context)), current_wavelength_index_(-1), total_ray_num_(0), active_ray_num_(0),
      buffer_size_(0), enter_ray_offset_(0) {}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
void Simulator::SetWavelengthIndex(int index) {
  if (index < 0 || index >= context_->wavelengths_.size()) {
    current_wavelength_index_ = -1;
    return;
  }

  current_wavelength_index_ = index;
}
#pragma clang diagnostic pop


// Start simulation
void Simulator::Start() {
  rays_.clear();
  exit_ray_segments_.clear();
  final_ray_segments_.clear();
  active_crystal_ctxs_.clear();
  RaySegmentPool::GetInstance()->Clear();
  enter_ray_data_.Clean();
  enter_ray_offset_ = 0;

  total_ray_num_ = context_->GetInitRayNum();
  InitSunRays();

  for (auto it = context_->multi_scatter_info_.begin(); it != context_->multi_scatter_info_.end(); ++it) {
    rays_.emplace_back();
    rays_.back().reserve(total_ray_num_);
    exit_ray_segments_.emplace_back();
    exit_ray_segments_.back().reserve(total_ray_num_ * 2);

    for (const auto& c : it->GetCrystalInfo()) {
      active_ray_num_ = static_cast<size_t>(c.population * total_ray_num_);
      if (buffer_size_ < total_ray_num_ * kBufferSizeFactor) {
        buffer_size_ = total_ray_num_ * kBufferSizeFactor;
        buffer_.Allocate(buffer_size_);
      }
      InitEntryRays(context_->GetCrystalContext(c.crystal_id));
      enter_ray_offset_ += active_ray_num_;
      TraceRays(context_->GetCrystal(c.crystal_id), context_->GetRayPathFilter(c.filter_id));
    }

    if (it != context_->multi_scatter_info_.end() - 1) {
      RestoreResultRays(it->GetProbability());  // total_ray_num_ is updated.
    }
    enter_ray_offset_ = 0;
  }

  for (const auto& r : exit_ray_segments_.back()) {
    final_ray_segments_.emplace_back(r);
  }
}


// Init sun rays, and fill into dir[1]. They will be rotated and fill into dir[0] in InitEntryRays().
// In world frame.
void Simulator::InitSunRays() {
  float sun_r = context_->sun_ctx_.GetSunDiameter() / 2;  // In degree
  const float* sun_ray_dir = context_->sun_ctx_.GetSunPosition();
  if (enter_ray_data_.ray_num < total_ray_num_) {
    enter_ray_data_.Allocate(total_ray_num_);
  }
  Math::RandomSampler::SampleSphericalPointsCart(sun_ray_dir, sun_r, enter_ray_data_.ray_dir, total_ray_num_);
  for (decltype(enter_ray_data_.ray_num) i = 0; i < enter_ray_data_.ray_num; i++) {
    enter_ray_data_.ray_seg[i] = nullptr;
  }
}


// Init entry rays into a crystal. Fill pt[0], face_id[0], w[0] and ray_seg[0].
// Rotate entry rays into crystal frame
// Add RayContext and main axis rotation
void Simulator::InitEntryRays(const CrystalContext* ctx) {
  auto& crystal = ctx->crystal;
  auto total_faces = crystal->TotalFaces();

  auto* prob = new float[total_faces];
  auto* face_norm = crystal->GetFaceNorm();
  auto* face_point = crystal->GetFaceVertex();
  auto* face_area = crystal->GetFaceArea();

  auto ray_pool = RaySegmentPool::GetInstance();

  float axis_rot[3];
  for (decltype(active_ray_num_) i = 0; i < active_ray_num_; i++) {
    InitMainAxis(ctx, axis_rot);
    Math::RotateZ(axis_rot, enter_ray_data_.ray_dir + (i + enter_ray_offset_) * 3, buffer_.dir[0] + i * 3);

    float sum = 0;
    for (int k = 0; k < total_faces; k++) {
      prob[k] = 0;
      if (!std::isnan(face_norm[k * 3 + 0]) && face_area[k] > 0) {
        prob[k] = std::max(-Math::Dot3(face_norm + k * 3, buffer_.dir[0] + i * 3) * face_area[k], 0.0f);
        sum += prob[k];
      }
    }
    for (int k = 0; k < total_faces; k++) {
      prob[k] /= sum;
    }

    buffer_.face_id[0][i] = Math::RandomSampler::SampleInt(prob, total_faces);
    Math::RandomSampler::SampleTriangularPoints(face_point + buffer_.face_id[0][i] * 9, buffer_.pt[0] + i * 3);

    auto prev_r = enter_ray_data_.ray_seg[enter_ray_offset_ + i];
    buffer_.w[0][i] = prev_r ? prev_r->w : 1.0f;

    auto r =
        ray_pool->GetRaySegment(buffer_.pt[0] + i * 3, buffer_.dir[0] + i * 3, buffer_.w[0][i], buffer_.face_id[0][i]);
    buffer_.ray_seg[0][i] = r;
    r->root_ctx = new RayInfo(r, ctx, axis_rot);
    r->root_ctx->prev_ray_segment = prev_r;
    rays_.back().emplace_back(r->root_ctx);
  }

  delete[] prob;
}


// Init crystal main axis.
// Random sample points on a sphere with given parameters.
void Simulator::InitMainAxis(const CrystalContext* ctx, float* axis) {
  auto rng = Math::RandomNumberGenerator::GetInstance();

  if (ctx->axis.latitude_dist == Math::Distribution::kUniform) {
    // Random sample on full sphere, ignore other parameters.
    Math::RandomSampler::SampleSphericalPointsSph(axis);
  } else {
    Math::RandomSampler::SampleSphericalPointsSph(ctx->axis, axis);
  }

  if (ctx->axis.roll_dist == Math::Distribution::kUniform) {
    // Random roll, ignore other parameters.
    axis[2] = rng->GetUniform() * 2 * Math::kPi;
  } else {
    axis[2] = rng->Get(ctx->axis.roll_dist, ctx->axis.roll_mean, ctx->axis.roll_std) * Math::kDegreeToRad;
  }
}


// Restore and shuffle resulted rays, and fill into dir[0].
void Simulator::RestoreResultRays(float prob) {
  if (buffer_size_ < exit_ray_segments_.back().size() * 2) {
    buffer_size_ = exit_ray_segments_.back().size() * 2;
    buffer_.Allocate(buffer_size_);
  }
  if (enter_ray_data_.ray_num < exit_ray_segments_.back().size()) {
    enter_ray_data_.Allocate(exit_ray_segments_.back().size());
  }

  auto rng = Math::RandomNumberGenerator::GetInstance();
  size_t idx = 0;
  for (const auto& r : exit_ray_segments_.back()) {
    if (!r->is_finished || r->w < context_->kScatMinW) {
      continue;
    }
    if (rng->GetUniform() > prob) {
      final_ray_segments_.emplace_back(r);
      continue;
    }
    const auto axis_rot = r->root_ctx->main_axis_rot.val();
    Math::RotateZBack(axis_rot, r->dir.val(), enter_ray_data_.ray_dir + idx * 3);
    enter_ray_data_.ray_seg[idx] = r;
    idx++;
  }
  total_ray_num_ = idx;

  // Shuffle
  for (decltype(total_ray_num_) i = 0; i < total_ray_num_; i++) {
    int tmp_idx = Math::RandomSampler::SampleInt(static_cast<int>(total_ray_num_ - i));

    float tmp_dir[3];
    std::memcpy(tmp_dir, enter_ray_data_.ray_dir + (i + tmp_idx) * 3, sizeof(float) * 3);
    std::memcpy(enter_ray_data_.ray_dir + (i + tmp_idx) * 3, enter_ray_data_.ray_dir + i * 3, sizeof(float) * 3);
    std::memcpy(enter_ray_data_.ray_dir + i * 3, tmp_dir, sizeof(float) * 3);

    RaySegment* tmp_r = enter_ray_data_.ray_seg[i + tmp_idx];
    enter_ray_data_.ray_seg[i + tmp_idx] = enter_ray_data_.ray_seg[i];
    enter_ray_data_.ray_seg[i] = tmp_r;
  }
}


// Trace rays.
// Start from dir[0] and pt[0].
void Simulator::TraceRays(const Crystal* crystal, AbstractRayPathFilter* filter) {
  auto pool = ThreadingPool::GetInstance();

  int max_recursion_num = context_->GetRayHitNum();
  float n = IceRefractiveIndex::Get(context_->wavelengths_[current_wavelength_index_].wavelength);
  for (int i = 0; i < max_recursion_num; i++) {
    if (buffer_size_ < active_ray_num_ * 2) {
      buffer_size_ = active_ray_num_ * kBufferSizeFactor;
      buffer_.Allocate(buffer_size_);
    }
    auto step = std::max(active_ray_num_ / 100, static_cast<size_t>(10));
    for (decltype(active_ray_num_) j = 0; j < active_ray_num_; j += step) {
      decltype(active_ray_num_) current_num = std::min(active_ray_num_ - j, step);
      pool->AddJob([=] {
        Optics::HitSurface(crystal, n, current_num,                                              //
                           buffer_.dir[0] + j * 3, buffer_.face_id[0] + j, buffer_.w[0] + j,     //
                           buffer_.dir[1] + j * 6, buffer_.w[1] + j * 2);                        // output
        Optics::Propagate(crystal, current_num * 2, buffer_.pt[0] + j * 3,                       //
                          buffer_.dir[1] + j * 6, buffer_.w[1] + j * 2, buffer_.face_id[0] + j,  //
                          buffer_.pt[1] + j * 6, buffer_.face_id[1] + j * 2);
      });
    }
    pool->WaitFinish();
    StoreRaySegments(crystal, filter);
    RefreshBuffer();  // active_ray_num_ is updated.
  }
}


// Save rays
void Simulator::StoreRaySegments(const Crystal* crystal, AbstractRayPathFilter* filter) {
  filter->ApplySymmetry(crystal);
  auto ray_pool = RaySegmentPool::GetInstance();
  for (size_t i = 0; i < active_ray_num_ * 2; i++) {
    if (buffer_.w[1][i] <= 0) {  // Refractive rays in total reflection case
      continue;
    }

    auto r = ray_pool->GetRaySegment(buffer_.pt[0] + i / 2 * 3, buffer_.dir[1] + i * 3, buffer_.w[1][i],
                                     buffer_.face_id[0][i / 2]);
    if (buffer_.face_id[1][i] < 0) {
      r->is_finished = true;
    }

    auto prev_ray_seg = buffer_.ray_seg[0][i / 2];
    if (i % 2 == 0) {
      prev_ray_seg->next_reflect = r;
    } else {
      prev_ray_seg->next_refract = r;
    }
    r->prev = prev_ray_seg;
    r->root_ctx = prev_ray_seg->root_ctx;
    buffer_.ray_seg[1][i] = r;

    if (!filter->Filter(crystal, r)) {
      continue;
    }
    if (r->is_finished || r->w < ProjectContext::kPropMinW) {
      exit_ray_segments_.back().emplace_back(r);
    }
  }
}


// Squeeze data, copy into another buffer_ (from buf[1] to buf[0])
// Update active_ray_num_.
void Simulator::RefreshBuffer() {
  size_t idx = 0;
  for (size_t i = 0; i < active_ray_num_ * 2; i++) {
    if (buffer_.face_id[1][i] >= 0 && buffer_.w[1][i] > ProjectContext::kPropMinW) {
      std::memcpy(buffer_.pt[0] + idx * 3, buffer_.pt[1] + i * 3, sizeof(float) * 3);
      std::memcpy(buffer_.dir[0] + idx * 3, buffer_.dir[1] + i * 3, sizeof(float) * 3);
      buffer_.w[0][idx] = buffer_.w[1][i];
      buffer_.face_id[0][idx] = buffer_.face_id[1][i];
      buffer_.ray_seg[0][idx] = buffer_.ray_seg[1][i];
      idx++;
    }
  }
  active_ray_num_ = idx;
}


const std::vector<RaySegment*>& Simulator::GetFinalRaySegments() const {
  return final_ray_segments_;
}


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-compare"
void Simulator::SaveFinalDirections(const char* filename) {
  File file(context_->GetDataDirectory().c_str(), filename);
  if (!file.Open(OpenMode::kWrite | OpenMode::kBinary))
    return;

  if (current_wavelength_index_ < 0 || current_wavelength_index_ >= context_->wavelengths_.size()) {
    return;
  }

  auto& w = context_->wavelengths_[current_wavelength_index_];
  file.Write(static_cast<float>(w.wavelength));
  file.Write(w.weight);

  auto ray_num = final_ray_segments_.size();
  size_t idx = 0;
  auto* data = new float[ray_num * 4];  // dx, dy, dz, w

  float* curr_data = data;
  for (const auto& r : final_ray_segments_) {
    const auto axis_rot = r->root_ctx->main_axis_rot.val();
    assert(r->root_ctx);

    Math::RotateZBack(axis_rot, r->dir.val(), curr_data);
    curr_data[3] = r->w;
    curr_data += 4;
    idx++;
  }
  file.Write(data, idx * 4);
  file.Close();

  delete[] data;
}
#pragma clang diagnostic pop


void Simulator::PrintRayInfo() {
  std::stack<RaySegment*> s;
  for (const auto& rs : exit_ray_segments_) {
    for (const auto& r : rs) {
      auto p = r;
      while (p) {
        s.push(p);
        p = p->prev;
      }
      std::printf("%zu,0,0,0,0,0,-1\n", s.size());
      while (!s.empty()) {
        p = s.top();
        s.pop();
        std::printf("%+.4f,%+.4f,%+.4f,%+.4f,%+.4f,%+.4f,%+.4f\n",  //
                    p->pt.x(), p->pt.y(), p->pt.z(),                // point
                    p->dir.x(), p->dir.y(), p->dir.z(),             // direction
                    p->w);                                          // weight
      }
    }
  }
}

}  // namespace IceHalo
