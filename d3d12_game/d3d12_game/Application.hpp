#pragma once
#include "External/StepTimer.h"

namespace dxapp {

// 前方宣言
class Device;
//class SimplePolygon;

#pragma region 追加
class GeometoryMesh;
class BufferObject;
class BasicShader;
#pragma endregion

/*!
 * @brief ゲームアプリケーションクラス
 */
class Application {
 public:
  /*!
   * @brief デフォルトコンストラクタ
   */
  Application();

  /*!
   * @brief デストラクタ
   */
  virtual ~Application();

  // コピー、代入はさせない
  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  /*!
   * @brief 初期化
   * @param[in] hWnd ウィンドウハンドル
   * @param[in] screenWidth 描画スクリーンの幅
   * @param[in] screenHeight 描画スクリーンの高さ
   */
  void Initialize(HWND hWnd, std::uint32_t screenWidth,
                  std::uint32_t screenHeight);

  /*!
   * @brief ゲームの実行
   */
  void Run();

  /*!
   * @brief 終了処理
   */
  void Terminate();

 private:
  /*!
   * @brief ゲームの更新
   * @param[in] timer タイマー
   */
  void Update(const DX::StepTimer& timer);

  /*!
   * @brief ゲームの描画
   */
  void Render();

  /*!
   * @brief レンダーターゲットをクリア
   */
  void ClearRenderTarget();

  //! ウィンドウハンドル
  HWND hWnd_{nullptr};

  //! デバイスオブジェクト
  std::unique_ptr<Device> device_;

  //! タイマー
  DX::StepTimer timer_;

  struct Transform
  {
	  DirectX::XMFLOAT3 pos{};
	  DirectX::XMFLOAT3 rot{};
	  DirectX::XMFLOAT3 sca{};
	  DirectX::XMMATRIX world{};
	  Transform() { Reset(); }
	  void Reset()
	  {
		  pos = DirectX::XMFLOAT3{};
		  rot = DirectX::XMFLOAT3{};
		  sca = DirectX::XMFLOAT3{ 1.0f, 1.0f, 1.0f };
		  // XMMatrixIdentityで単位行列を取得できる
		  // 単位行列は何も作用しない行列
		  world = DirectX::XMMatrixIdentity();
	  }
  };

  struct CameraData
  {
	  DirectX::XMFLOAT3 eye;     //!< 視点
	  DirectX::XMFLOAT3 target;  //!< 注視点
	  DirectX::XMFLOAT3 up;      //! カメラの上ベクトル(傾き)
	  float aspect;              //!< 画面縦横比
	  float fov;                 //!< 視野角
	  float nearZ;  //!< Zのニアクリップ(これより近いと描画しない)
	  float farZ;  //!< Zのファークリップ(これより遠いと描画しなし)
	  DirectX::XMMATRIX view;      //! ビュー行列
	  DirectX::XMMATRIX proj;      //! 射影行列
	  DirectX::XMMATRIX viewProj;  //! ビュー射影行列

	  CameraData() { Reset(); }

	  void Reset() {
		  // カメラ設定
		  // 原点からちょい上して後ろに下がる
		  eye = DirectX::XMFLOAT3{ 0, 2.0f, -5.0f };
		  // 原点を見る
		  target = DirectX::XMFLOAT3{};
		  // カメラはY軸+が上方向
		  up = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.0f };

		  // プロジェクション設定
		  aspect = DirectX::XMConvertToRadians(60.f);
		  fov = 16.0f / 9.0f;
		  nearZ = 0.1f;
		  farZ = 1000.0f;

		  proj = view = DirectX::XMMatrixIdentity();
	  }

	  void CalculateViewMatrix() {
		  auto e = DirectX::XMLoadFloat3(&eye);
		  auto t = DirectX::XMLoadFloat3(&target);
		  auto u = DirectX::XMLoadFloat3(&up);

		  // XMMatrixLookAtLHで視点から注視点方向を向く行列ができる
		  view = DirectX::XMMatrixLookAtLH(e, t, u);
	  }

	  void CalculateProjMatrix() {
		  // XMMatrixPerspectiveFovLHで遠いものを小さく、
		  // 近いものを大きくするための行列(透視・パースペクティブ)が得られる
		  proj = DirectX::XMMatrixPerspectiveFovLH(aspect, fov, nearZ, farZ);
	  }
  };

  std::unique_ptr<BasicShader>m_BasicShader;
  // サンプラーのデスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SamplerHeap{};
  // 定数バッファとSRV(テクスチャ)のためのデスクリプタヒープ
  Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvHeap{};
  struct RenderObject {
	  // キューブ用のメッシュ1個ぶん用意してみる
	  std::unique_ptr<GeometoryMesh> mesh;
	  Transform trasnform;  // キューブのトランスフォーム

	  // 定数バッファはオブジェクト単位で用意するのがいいのでは。
	  // 静的なテクスチャやサンプラは書き換えられないのでリソースと1対1でよい。
	  // しかし！定数バッファはフレームごとにデータが変わる可能性がある
	  // GPUが処理前にデータを書き換えるとプログラムがクラッシュすることもあるので
	  // バックバッファと同じ数だけ確保して使うのが安全。
	  // こういうのをダブルバッファ化とかいいます。

	  // 定数バッファーそのもの
	  std::vector<std::unique_ptr<BufferObject>> cbuffer;
  };
  RenderObject m_CubeObj;

  void UpdateCube();
  void DrawCube();
  CameraData camera_{};  // カメラ
};
}  // namespace dxapp
