#ifndef __HEADER_SN_INDEXER__
#define __HEADER_SN_INDEXER__

#include <core/types.h>
#include <vector>

namespace sn
{

/// \brief References object and gives them a two-parts ID
template <typename T>
class Indexer
{
public:
	struct Key
	{
		/// \brief Zero-based index suitable for vector storage
		u32 i;
		/// \brief Version number used to disambiguate similar indexes.
		/// The i and v combo form an ID suitable within the process's scope.
		u32 v;
	};
	
	Key add(T elem)
	{
		u32 i = 0;
		while(i < m_elems.size())
		{
			if(m_elems[i] == nullptr)
			{
				m_elems[i] = elem;
				++m_versions[i];
				return {i, m_versions[i]};
			}
			++i;
		}
		m_elems.push_back(elem);
		m_versions.push_back(0);
		return {i, m_versions.back()};
	}
	
	T get(Key k)
	{
		if(k.i < m_elems.size() && k.v == m_versions[k.i])
			return m_elems[k.i];
		return nullptr;
	}

    /// \brief Tests if the given item is stored in the container.
    /// Warning: it's O(n).
    /// \param elem
    bool contains(T elem)
    {
        for (auto it = m_elems.begin(); it != m_elems.end(); ++it)
        {
            if (*it == elem)
                return true;
        }
        return false;
    }
	
	T remove(Key k)
	{
        if (get(k))
        {
            T elem = m_elems[k.i];
			m_elems[k.i] = T();
            return elem;
        }
        return T();
	}
	
	const std::vector<T> & getRawElements() const { return m_elems; }
    const std::vector<u32> & getRawVersions() const { return m_versions; }
	
private:
	std::vector<T> m_elems;
	std::vector<u32> m_versions;
};

} // namespace sn

#endif // __HEADER_SN_INDEXER__


