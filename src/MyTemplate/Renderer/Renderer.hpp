#pragma once // potentially faster compile-times if supported
#ifndef RENDERER_HPP_YBLYHOXN
#define RENDERER_HPP_YBLYHOXN

#include "MyTemplate/Renderer/common.hpp"	
#include <memory>

namespace gfx {
	class Renderer final {
		public:
			struct State; // forward declaration // TODO: expose instead?
			Renderer();
			Renderer( Renderer const &  ) = delete;
			Renderer( Renderer       && ) noexcept;
			~Renderer()                   noexcept;
			// TODO: update, draw, load_texture, etc?
			[[nodiscard]] Window const & get_window() const;
			[[nodiscard]] Window       & get_window()      ;
			void                         operator()()      ; // render
		private:
			std::unique_ptr<State> m_p_state;
	}; // end-of-class: Renderer
} // end-of-namespace: gfx

#endif // end-of-header-guard RENDERER_HPP_YBLYHOXN
// EOF
