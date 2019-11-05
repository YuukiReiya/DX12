#pragma once
#include "Singleton.hpp"

namespace dxapp {
	class Device;

	class TextureManager {
	public:
		/*!
			@brief	コンストラクタ
		*/
		TextureManager();
		/*!
			@brief	デストラクタ
		*/
		~TextureManager();

		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;
		TextureManager(TextureManager&&) = delete;
		TextureManager& operator=(TextureManager&&) = delete;

		/*!
			@brief オブジェクトの破棄。SingletonFinalizerが呼び出す
		*/
		bool LoadWICTextureFromFile(Device* device, const std::wstring& fileName, const std::string& assetName);

		/*!
			@brief	オブジェクトの破棄
			@detail	SingletonFinalizerが呼び出す
		*/
		Microsoft::WRL::ComPtr<ID3D12Resource> texture(const std::string& assetName);
	private:
		// これはpImplパターン
		// 詳細な実装は内部クラスImplになげる
		// これにより外部のプログラム対してがっつり隠蔽できる
		class Impl;
		std::unique_ptr<Impl> impl_;  //!< TextureManagerの実装
	};
}