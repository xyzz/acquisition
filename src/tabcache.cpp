#include "tabcache.h"
#include "QsLog.h"
#include "itemlocation.h"

#include <QDir>
#include <QDateTime>

// TabCache
//
// This class is a 'work around' for path of exile API not using 'ETag' headers
// and supporting existing cache methods that QNetworkDiskCache would have
// otherwise supported directly.  If they ever fix this we should be able to
// just remove this implementation and use QNetworkDiskCache directly.
//
// Currently the API sends headers that look like this:
//
// DEBUG 2016-01-16T13:04:10.054 "Server" "nginx/1.4.4"
// DEBUG 2016-01-16T13:04:10.054 "Date" "Sat, 16 Jan 2016 19:04:10 GMT"
// DEBUG 2016-01-16T13:04:10.054 "Content-Type" "application/json"
// DEBUG 2016-01-16T13:04:10.054 "Expires" "Thu, 19 Nov 1981 08:52:00 GMT"
// DEBUG 2016-01-16T13:04:10.055 "Cache-Control" "no-store, no-cache, must-revalidate, post-check=0, pre-check=0"
// DEBUG 2016-01-16T13:04:10.055 "Pragma" "no-cache"
// DEBUG 2016-01-16T13:04:10.055 "X-Frame-Options" "SAMEORIGIN"
//
// 'no-cache' - actually allows client to cache requests, but *requests*
// revalidation to use cached data (using ETag or Last Modified headers).
// clients can choose to ignore revalidation request and use possibly
// stale content.
//
// 'must-revalidate' - *requires* revalidation of cached content.  If no
// revalidation method exists don't use cached data.
//
// 'no-store' - *requires* client to not store content to disk.  This is the what
// really disables caching.
//
// So the plan is to basically just to ignore the Cache-Control and Pragma headers.
// This is achieved by overridding QNetworkDiskCache 'prepare' method so it
// effectively doesn't seem them.
//
// A helper method is needed however to create our network requests properly such
// such that the requests always prefer hitting in the cache.
//
// The method is:
//
// Request(url, <flags>);
//
// Where <flags> can be used to force an eviction, causing a reload
//   Request(url, TabCache::Refresh);
//
// If 'Refresh' is not specified we're guaranteed to hit in cache if entry exists
// and fetch otherwise.
//
// So, to minimize overall API requests it will become a matter of minimizing requests
// that ask for a refresh.  From the point of view of the rest of the application it will
// always appear as if we request all the tabs.

TabCache::TabCache(QObject* parent)
    :QNetworkDiskCache(parent)
{
}

QNetworkRequest TabCache::Request(const QUrl &url, const ItemLocation &location, Flags flags) {
    QNetworkRequest request{url};

    // The cache policy exists so it can override normal behavior for a given refresh.
    // Based on the current policy we may ignore refresh requests, or force refreshes
    // even if none was specifed with 'flags'.
    switch (cache_policy_) {
    case DefaultCache:
        // This is the default policy, where we honor cache policy as specified by 'flags'
        // Evict this request from the cache if refresh is requested
        if (flags.testFlag(Refresh))
            remove(url);
        break;
    case AlwaysCache:
        // By default we always try to hit in the cache, so this policy just allows
        // the normal cache mechanism to do its job and it will basically grab everything
        // from the cache that is available for all network requests
        break;
    case NeverCache:
        // We've already fully flushed the cache in OnPolicyUpdate, so nothing to do here.
        break;
    case ManualCache:
        // The case involves refreshing only an explicitily specified set of named tabs, customers
        // use AddManualRefresh to indicate what set of tabs to refresh before triggering a refresh
        // all other network requests will come from the cache if available
        if (location.IsValid() && manual_refresh_.count(location.GetUniqueHash()))
            remove(url);
        break;
    default:
        QLOG_ERROR() << "TabCache::Request Failed to handle all cache policy cases";
        break;
    };

    // At this point we've evicted any request that should be refreshed, so we always
    // tell the 'real' request to prefer but not require the entry be in the cache.
    // If it is not in the cache it will be fetched from the network regardless.
    request.setAttribute(QNetworkRequest::CacheSaveControlAttribute, QNetworkRequest::PreferCache);

    return request;
}


void TabCache::OnPolicyUpdate(Policy policy) {
    // Handle NeverCache policy here, basically just flush cache.
    if (policy == NeverCache)
        clear();

    cache_policy_ = policy;
}

void TabCache::AddManualRefresh( const ItemLocation &location) {
    if (location.IsValid())
        manual_refresh_.insert(location.GetUniqueHash());
}


void TabCache::OnItemsRefreshed() {
    // Clear any manually set tabs here
    manual_refresh_.clear();
}

QIODevice *TabCache::prepare(const QNetworkCacheMetaData &metaData) {
    QLOG_DEBUG() << "TabCache::prepare";
    QNetworkCacheMetaData local{metaData};

    //Default policy based on received HTTP headers is to not save to disk.
    //Override here, and set proper expiration so items are put in cache
    //(setSaveToDisk) and valid when retrieved using metaData call -
    //(setExpirationDate)

    local.setSaveToDisk(true);

    // Need to set some reasonable length of time in which our cache entries
    // will expire.  It's possible we'll want to allow users to customize this.
    local.setExpirationDate(QDateTime().currentDateTime().addDays(kCacheExpireInDays));

    QNetworkCacheMetaData::RawHeaderList headers;

    for (auto &header: local.rawHeaders()) {
        // Modify Cache-Control headers - basically need to drop 'no-store'
        // as we want to store to cache and 'must-revalidate' as we don't have
        // ETag or last modified headers available to attempt re-validation. To
        // be on the safe side though just drop Cache-Control and Pragma headers.

        if (header.first == "Cache-Control") continue;
        if (header.first == "Pragma") continue;

        headers.push_back(header);
    }
    local.setRawHeaders(headers);

    return QNetworkDiskCache::prepare(local);
}

