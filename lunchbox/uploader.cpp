
/* Copyright (C) 2013, Stefan.Eilemann@epfl.ch
 *
 * This file is part of Lunchbox <https://github.com/Eyescale/Lunchbox>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 2.1 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "uploader.h"

#include "plugin.h"
#include "pluginInstance.h"
#include "pluginRegistry.h"
#include "pluginVisitor.h"

namespace lunchbox
{
namespace detail
{
class Uploader : public PluginInstance
{
public:
    Uploader() : PluginInstance( EQ_COMPRESSOR_NONE ), gl( 0 ) {}

    Uploader( lunchbox::PluginRegistry& registry, const uint32_t name,
              const GLEWContext* gl_ )
        : PluginInstance( name )
        , gl( gl_ )
    {
        if( name <= EQ_COMPRESSOR_NONE )
            return;

        LBASSERT( gl );
        plugin = registry.findPlugin( name );
        LBASSERT( plugin );
        if( !plugin )
            return;

        instance = plugin->newCompressor( name );
        info = plugin->findInfo( name );
        LBASSERT( instance );
        LBASSERT( info.name == name );
        LBASSERT( info.capabilities & EQ_COMPRESSOR_TRANSFER );
        LBLOG( LOG_PLUGIN ) << "Instantiated uploader of type 0x" << std::hex
                            << name << std::dec << std::endl;
    }

    ~Uploader()
    {
        if( instance )
            plugin->deleteDecompressor( instance );
        instance = 0;
        plugin = 0;
    }

    bool isGood() const { return gl && PluginInstance::isGood(); }

    const GLEWContext* const gl;
};
}

Uploader::Uploader()
    : impl_( new detail::Uploader )
{
    LB_TS_THREAD( _thread );
}

Uploader::Uploader( PluginRegistry& registry, const uint32_t name,
                    const GLEWContext* gl )
    : impl_( new detail::Uploader( registry, name, gl ))
{
    LB_TS_THREAD( _thread );
}

Uploader::Uploader( PluginRegistry& registry, const uint32_t externalFormat,
                    const uint32_t internalFormat, const uint64_t capabilities,
                    const GLEWContext* gl )
    : impl_( new detail::Uploader( registry,
                                   choose( registry, externalFormat,
                                           internalFormat, capabilities, gl ),
                                   gl ))
{
    LB_TS_THREAD( _thread );
}


Uploader::~Uploader()
{
    LBASSERTINFO( impl_->plugin == 0,
                  "Clear downloader while GL context is still active" );
    delete impl_;
}

bool Uploader::isGood() const
{
    LB_TS_SCOPED( _thread );
    return impl_->isGood() && impl_->instance &&
        impl_->plugin->isCompatible( impl_->info.name, impl_->gl );
}

bool Uploader::uses( const uint32_t name ) const
{
    return impl_->isGood() && impl_->instance && impl_->info.name == name;
}

bool Uploader::supports( const uint32_t externalFormat,
                         const uint32_t internalFormat,
                         const uint64_t capabilities ) const
{
    return isGood() && impl_->info.outputTokenType == externalFormat &&
           (impl_->info.capabilities & capabilities) == capabilities &&
           impl_->info.tokenType == internalFormat;
}

namespace
{
class Finder : public ConstPluginVisitor
{
public:
    Finder( const uint32_t externalFormat, const uint32_t internalFormat,
            const uint64_t capabilities, const GLEWContext* gl )
        : externalFormat_( externalFormat )
        , internalFormat_( internalFormat )
        , capabilities_( capabilities )
        , gl_( gl )
    {
        current.name = EQ_COMPRESSOR_NONE;
        current.speed = 0.f;
    }

    virtual VisitorResult visit( const Plugin& plugin,
                                 const EqCompressorInfo& info )
    {
        if( (info.capabilities & capabilities_) == capabilities_ &&
            info.outputTokenType == externalFormat_ &&
            info.tokenType == internalFormat_ &&
            plugin.isCompatible( info.name, gl_ ) &&
            current.speed < info.speed )
        {
            current = info;
        }
        return TRAVERSE_CONTINUE;
    }

    EqCompressorInfo current;
private:
    const uint32_t externalFormat_;
    const uint32_t internalFormat_;
    const uint64_t capabilities_;
    const GLEWContext* gl_;
};
}

uint32_t Uploader::choose( const PluginRegistry& registry,
                           const uint32_t externalFormat,
                           const uint32_t internalFormat,
                           const uint64_t capabilities, const GLEWContext* gl )
{
    Finder finder( externalFormat, internalFormat, capabilities, gl );
    registry.accept( finder );
    return finder.current.name;
}

const EqCompressorInfo& Uploader::getInfo() const
{
    return impl_->info;
}

void Uploader::swap( Uploader& other )
{
    std::swap( impl_, other.impl_ );
}

void Uploader::clear()
{
    delete impl_;
    impl_ = new detail::Uploader;
}

void Uploader::upload( const void* buffer, const uint64_t inDims[4],
                       const uint64_t flags, const uint64_t outDims[4],
                       const unsigned destination )
{
    LBASSERT( isGood( ));
    impl_->plugin->upload( impl_->instance, impl_->info.name, impl_->gl,
                           buffer, inDims, flags, outDims, destination );
}

}
