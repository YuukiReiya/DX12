#pragma once
namespace dxapp {
	class Device;
	/*!
	 * @brief LightingVS / PSを動かすためのコード
	 */
	class LightingShader {
	public:
		/*!
		 * @brief オブジェクト単位で用意するべき構造体
		 *       （定数バッファのObjectParamと同じ）
		 */
		struct ObjectParam {
			DirectX::XMFLOAT4X4 world;
			DirectX::XMFLOAT4X4 texTrans;
		};

		//! シェーダライト対応数と合わせる
		static constexpr int MaxDirLightNum{ 3 };
		/*!
		 * @brief 平行光源用ライト構造体.平行光源なので位置は持たない
		 *        (定数バッファと同じ内容)
		 */
		struct Light {
			DirectX::XMFLOAT3 strength{ 0.5f, 0.5f, 0.5f };  //!< ライトの色と強さ
			float pad1;  //!< メモリの並びを整える用
			DirectX::XMFLOAT3 direction{ 0, -1, 0 };  //!< 光源の向き
			float pad2;  //!< メモリの並びを整える用
		};

		/*!
		 * @brief シーン共通のパラメータをかき集めた構造体
		 *        (定数バッファと同じ内容)
		 */
		struct SceneParam {
			DirectX::XMFLOAT4X4 view;      //!< ビュー行列
			DirectX::XMFLOAT4X4 proj;      //!< 射影行列
			DirectX::XMFLOAT4X4 viewProj;  //!< ビュー射影
			DirectX::XMFLOAT3 eyePos;      //!< 視点
			float pad;                     //!< メモリの並びを整える用
			DirectX::XMFLOAT4 ambientLight;  //!< 環境光(空間を満たしている一定の明るさ)
			Light lights[MaxDirLightNum];  //!< ライトの配列
		};

	public:
		/*!
		 * @brief コンストラクタ
		 */
		LightingShader();

		/*!
		 * @brief デストラクタ
		 */
		~LightingShader();

		/*!
		 * @brief 初期化
		 */
		void Initialize(Device* device);

		/*!
		 * @brief 終了処理
		 */
		void Terminate();

		/*!
		 * @brief フレームでシェーダを使用するときに最初に呼び出す関数
		 * @details Endを呼ぶまでは引数のコマンドリストを使う
		 * @param[in,ont] commandList コマンドを積むためのコマンドリスト
		 */
		void Begin(ID3D12GraphicsCommandList* commandList);

		/*!
		 * @brief フレームでシェーダを使い追わったら必ず呼ぶ関数
		 */
		void End();

		/*!
		 * @brief シェーダが持っているパラメータでコマンドを発行
		 */
		void Apply();

		/*!
		 * @brief 定数バッファb0を設定する
		 */
		void SetObjectParam(D3D12_GPU_VIRTUAL_ADDRESS addr);

		/*!
		 * @brief 定数バッファb1を設定する
		 */
		void SetSceneParam(D3D12_GPU_VIRTUAL_ADDRESS addr);

		/*!
		 * @brief 定数バッファb2を設定する
		 */
		void SetMaterialParam(D3D12_GPU_VIRTUAL_ADDRESS addr);

		/*!
		 * @brief 参照するSRVのデスクリプタヒープ
		 * @param[in] heaps デスクリプタヒープ
		 * @param[in] offset ヒープ内でのオフセット位置
		 */
		void SetSrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

		/*!
		 * @brief 参照するサンプラのデスクリプタヒープ
		 * @param[in] heaps デスクリプタヒープ
		 * @param[in] offset ヒープ内でのオフセット位置
		 */
		void SetSamplerDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

		/*!
		 * @brief ダミーテクスチャの設定
		 * @param[in] heaps デスクリプタヒープ
		 * @param[in] offset ヒープ内でのオフセット位置
		 */
		void SetDammySrvDescriptorHeap(ID3D12DescriptorHeap* heap, const int offset);

		/*!
		 * @brief デフォルトとして使うサンプラのデスクリプタヒープ
		 * @param[in] heaps デスクリプタヒープ
		 * @param[in] offset ヒープ内でのオフセット位置
		 */
		void SetDefaultSamplerDescriptorHeap(ID3D12DescriptorHeap* heap,
			const int offset);

	private:
		//! 内部実装クラス
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
}  // namespace dxapp