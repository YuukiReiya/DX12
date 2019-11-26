#pragma once

namespace dxapp {
	class Device;

	/*!
	 * @brief ShadowMapVS / PSを動かすためのコード
	 *        デプスだけを記録するのでほかのシェーダーよりもシンプル
	 */

	class ShadowMapShader {
	public:
		/*!
		 * @brief コンストラクタ
		 */
		ShadowMapShader();

		/*!
		 * @brief デストラクタ
		 */
		~ShadowMapShader();

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

	private:
		//! 内部実装クラス
		class Impl;
		std::unique_ptr<Impl> impl_;
	};
}  // namespace dxapp