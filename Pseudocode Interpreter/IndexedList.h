#pragma once
#include <assert.h>
#define INDEXEDLIST_SKIP 10 // 跳跃长度

template<typename Type>
class IndexedList
{
protected:
	struct Node {
		Node* previous;
		Node* next;
		Type value;
		bool end = false;
	};
	struct IndexNode {
		IndexNode* next;
		union {
			IndexNode* target;
			Node* value;
		};
		UINT index;
		bool is_index;
	};
public:
	class iterator
	{
	protected:
		Node* m_Node;
	public:
		iterator() {
			m_Node = nullptr;
		}
		iterator(Node* init) {
			m_Node = init;
		}
		~iterator() {}
		void operator =(const iterator& iter) {
			m_Node = iter.m_Node;
		}
		bool operator ==(const iterator& iter) {
			return m_Node == iter.m_Node;
		}
		bool operator !=(const iterator& iter) {
			return m_Node != iter.m_Node;
		}
		void operator --(int arg) {
			assert(m_Node->previous);
			m_Node = m_Node->previous;
		}
		void operator ++(int arg) {
			assert(m_Node);
			m_Node = m_Node->next;
		}
		iterator operator ++() {
			assert(m_Node);
			m_Node = m_Node->next;
			return iterator(m_Node);
		}
		Type& operator *() {
			return m_Node->value;
		}
		Type* operator ->() {
			return &m_Node->value;
		}
	};
protected:
	Node* m_Root;
	Node* m_ReverseRoot;
	IndexNode* m_IndexRoot;
	UINT m_Size;
	USHORT m_Depth;
	bool m_ConstructionMode = false; // 构建模式（将操作限定于append，添加的同时构建索引层）
	IndexNode** m_ConstructionStack = nullptr; // 仅在构建模式下是哟
public:
	IndexedList() {
		m_ReverseRoot = new Node{ nullptr, nullptr, (Type)NULL, true };
		m_Root = m_ReverseRoot;
		m_IndexRoot = nullptr;
		m_Size = 0;
		m_Depth = 0;
	}
	~IndexedList() {
		clear();
		delete m_ReverseRoot;
	}
	UINT size() const {
		return m_Size;
	}
	iterator begin() const {
		return iterator(m_Root);
	}
	iterator end() const {
		return iterator(m_ReverseRoot);
	}
	iterator operator [](INT index) const {
		if (index < 0) {
			index = index + m_Size;
		}
		assert(index >= 0 and index < m_Size);
		IndexNode* this_index_node = m_IndexRoot;
		while (true) {
			if (this_index_node->next) {
				if (this_index_node->next->index > index) {
					if (this_index_node->is_index) {
						this_index_node = this_index_node->target;
					}
					else {
						break;
					}
				}
				else {
					this_index_node = this_index_node->next;
				}
			}
			else {
				if (this_index_node->is_index) {
					this_index_node = this_index_node->target;
				}
				else {
					break;
				}
			}
		}
		Node* this_node = this_index_node->value;
		UINT difference = index - this_index_node->index;
		for (UINT count = 0; count != difference; count++) {
			this_node = this_node->next;
		}
		return iterator(this_node);
	}
	void append(Type value) {
		Node* new_node = new Node{ m_ReverseRoot->previous, m_ReverseRoot, value };
		if (m_ConstructionMode) {
			if (m_Size == 0) {
				m_Root = new_node;
				m_IndexRoot = new IndexNode{ nullptr, (IndexNode*)new_node, 0, false };
				m_Depth = 1;
				m_ConstructionStack = new IndexNode*[] { m_IndexRoot };
			}
			else {
				m_ReverseRoot->previous->next = new_node;
				// 如果达到了跳跃长度，则更新索引节点
				USHORT level;
				for (level = 0; level != m_Depth;) {
					if (m_Size % (UINT)pow(INDEXEDLIST_SKIP, level + 1) == 0) {
						level++;
					}
					else {
						break;
					}
				}
				// 若达到新的数量级，则重新分配栈
				if (level == m_Depth) {
					IndexNode** new_stack = new IndexNode* [level + 1];
					memcpy(new_stack, m_ConstructionStack, sizeof(IndexNode*) * m_Depth);
					delete[] m_ConstructionStack;
					m_ConstructionStack = new_stack;
					m_ConstructionStack[m_Depth] = new IndexNode{ nullptr, m_ConstructionStack[m_Depth - 1], 0, true };
					m_IndexRoot = m_ConstructionStack[m_Depth];
					m_Depth++;
				}
				IndexNode* lower_level = (IndexNode*)new_node;
				for (USHORT current_level = 0; current_level != level; current_level++) {
					IndexNode* new_index_node = new IndexNode{ nullptr, lower_level, m_Size, true };
					m_ConstructionStack[current_level]->next = new_index_node;
					m_ConstructionStack[current_level] = new_index_node;
					lower_level = new_index_node;
				}
				m_ConstructionStack[0]->is_index = false;
			}
		}
		else {
			if (m_Size == 0) {
				m_Root = new_node;
				m_IndexRoot = new IndexNode{ nullptr, (IndexNode*)new_node, 0, false };
				m_Depth = 1;
			}
			else {
				m_ReverseRoot->previous->next = new_node;
			}
		}
		m_ReverseRoot->previous = new_node;
		m_Size++;
	}
	void insert(UINT index, Type value) {
		assert(m_ConstructionMode == false);
		assert(index >= 0 and index <= m_Size);
		if (index == m_Size) {
			append(value);
			return;
		}
		IndexNode** stack = new IndexNode* [m_Depth] {nullptr}; // 用于更新之后节点的索引
		USHORT stack_ptr = 0;
		IndexNode* this_index_node = m_IndexRoot;
		Node* this_node = m_Root;
		if (this_index_node) {
			while (true) {
				if (this_index_node->next) {
					if (this_index_node->next->index > index) {
						stack[stack_ptr] = this_index_node;
						stack_ptr++;
						if (this_index_node->is_index) {
							this_index_node = this_index_node->target;
						}
						else {
							break;
						}
					}
					else {
						this_index_node = this_index_node->next;
					}
				}
				else {
					if (this_index_node->is_index) {
						this_index_node = this_index_node->target;
					}
					else {
						stack[stack_ptr] = this_index_node;
						stack_ptr++;
						break;
					}
				}
			}
			this_node = this_index_node->value;
			UINT difference = index - this_index_node->index;
			for (UINT count = 0; count != difference; count++) {
				this_node = this_node->next;
			}
			// 插入
			Node* new_node;
			if (index == 0) {
				new_node = new Node{ nullptr, this_node, value };
				this_node->previous = new_node;
				m_Root = new_node;
			}
			else {
				new_node = new Node{ this_node->previous, this_node, value };
				this_node->previous->next = new_node;
				this_node->previous = new_node;
			}
			m_Size++;
			// 更新索引
			for (USHORT stack_index = 0; stack_index != stack_ptr; stack_index++) {
				this_index_node = stack[stack_index]->next;
				while (this_index_node) {
					this_index_node->index++;
					this_index_node = this_index_node->next;
				}
			}
			if (index == stack[stack_ptr - 1]->index) {
				stack[stack_ptr - 1]->value = new_node;
			}
		}
		else {
			// 空链表
			m_Root = new Node{ nullptr, m_ReverseRoot, value, false };
			m_ReverseRoot->previous = m_Root;
			m_IndexRoot = new IndexNode{ nullptr, (IndexNode*)m_Root, 0, false };
			m_Depth = 1;
		}
	}
	Type pop(UINT index) {
		assert(index >= 0 and index < m_Size);
		assert(m_Size != 0);
		IndexNode** stack = new IndexNode* [m_Depth] {nullptr}; // 用于更新之后节点的索引
		USHORT stack_ptr = 0;
		IndexNode* this_index_node = m_IndexRoot;
		Node* this_node;
		while (true) {
			if (this_index_node->next) {
				if (this_index_node->next->index > index) {
					stack[stack_ptr] = this_index_node;
					stack_ptr++;
					if (this_index_node->is_index) {
						this_index_node = this_index_node->target;
					}
					else {
						break;
					}
				}
				else {
					this_index_node = this_index_node->next;
				}
			}
			else {
				if (this_index_node->is_index) {
					this_index_node = this_index_node->target;
				}
				else {
					stack[stack_ptr] = this_index_node;
					stack_ptr++;
					break;
				}
			}
		}
		this_node = this_index_node->value;
		UINT difference = index - this_index_node->index;
		for (UINT count = 0; count != difference; count++) {
			this_node = this_node->next;
		}
		if (index == 0) {
			stack[stack_ptr - 1]->value = m_Root = this_node->next;
			this_node->next->previous = m_Root;
			clear();
		}
		else {
			// 当索引指向的节点被移除
			if (index == stack[stack_ptr - 1]->index) {
				stack[stack_ptr - 1]->value = this_node->previous;
				for (USHORT stack_index = 0; stack_index != stack_ptr; stack_index++) {
					stack[stack_index]->index--;
				}
			}
			// 移除
			this_node->previous->next = this_node->next;
			this_node->next->previous = this_node->previous;
		}
		Type value = this_node->value;
		delete this_node;
		m_Size--;
		// 更新索引
		for (USHORT stack_index = 0; stack_index != stack_ptr; stack_index++) {
			this_index_node = stack[stack_index]->next;
			while (this_index_node) {
				this_index_node->index--;
				this_index_node = this_index_node->next;
			}
		}
		return value;
	}
	void clear() {
		auto release = [](IndexNode* current) {
			while (current) {
				IndexNode* next = current->next;
				delete current;
				current = next;
			}
		};
		if (m_Size == 0) { return; }
		IndexNode** stack = new IndexNode * [m_Depth];
		USHORT stack_ptr = 0;
		IndexNode* this_index_node = m_IndexRoot;
		// 获取所有层索引起始点
		while (true) {
			stack[stack_ptr] = this_index_node;
			stack_ptr++;
			if (this_index_node->is_index) {
				this_index_node = this_index_node->target;
			}
			else {
				break;
			}
		}
		// 删除所有索引
		for (USHORT level = 0; level != stack_ptr; level++) {
			release(stack[level]);
		}
		delete[] stack;
		// 删除所有节点
		Node* current = m_Root;
		while (current) {
			Node* next = current->next;
			delete current;
			current = next;
		}
		m_ReverseRoot = new Node{ nullptr, nullptr, (Type)NULL, true };
		m_Root = m_ReverseRoot;
		m_IndexRoot = nullptr;
		m_Size = 0;
		m_Depth = 0;
	}
	inline void set_construction(bool state) {
		m_ConstructionMode = state;
		if (state) {
			assert(m_Size == 0 or m_Size == 1); // 只有一个元素的链表的索引层也是有效的
			if (m_Size == 1) {
				m_ConstructionStack = new IndexNode* [] {m_IndexRoot};
			}
		}
		else {
			delete[] m_ConstructionStack;
		}
	}
};